#!/bin/bash
set -uo pipefail

# SMS command dispatcher.
# The first SMS word is the command name. Remaining words are parameters parsed
# by command modules loaded from commands.d/*.sh. SMS text is never executed as
# shell code.

# Access levels:
#   0 - blocked
#   1 - user
#   2 - supervisor
#   3 - admin
declare -A user_access

# User list.
# Add real phone numbers here. Keep numbers in international format.
# Level 0 blocks a known number without treating it as unknown.
# user_access["+19991111111"]=1  # User: low-risk read-only commands.
# user_access["+19992222222"]=2  # Supervisor: networking commands.
# user_access["+19993333333"]=3  # Admin: dangerous commands such as reboot.
# user_access["+15550000000"]=3  # Example admin number.

# Runtime knobs. Override these from the environment when testing or installing.
SMS_SEND=${SMS_SEND:-sms-send}
LOG_FILE=${SMS_CMD_LOG:-/tmp/sms-daemon/sms-cmd.log}
DRY_RUN=${SMS_CMD_DRY_RUN:-0}
COMMAND_TIMEOUT=${SMS_CMD_COMMAND_TIMEOUT:-45}
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
COMMAND_DIR=${SMS_CMD_COMMAND_DIR:-${SCRIPT_DIR}/commands.d}

if ! [[ "$COMMAND_TIMEOUT" =~ ^[0-9]+$ ]] || (( COMMAND_TIMEOUT < 1 )); then
    COMMAND_TIMEOUT=45
fi

request=${1:-}
incoming_number=${2:-}
sent_time=${3:-}
receive_time=${4:-}
smsc=${5:-}
imsi=${6:-}
imei=${7:-}
level=0

declare -A command_handlers
declare -A command_levels
declare -A command_help
declare -a command_order

register_command() {
    local name=$1
    local handler=$2
    local required_level=$3
    local help_text=${4:-$name}

    if [[ -z "$name" || -z "$handler" || -z "$required_level" ]]; then
        echo "Invalid command registration" >&2
        exit 1
    fi
    if ! declare -F "$handler" >/dev/null; then
        echo "Command handler is not defined: $handler" >&2
        exit 1
    fi
    if ! [[ "$required_level" =~ ^[0-9]+$ ]]; then
        echo "Invalid access level for command $name: $required_level" >&2
        exit 1
    fi

    if [[ -z "${command_handlers[$name]+set}" ]]; then
        command_order+=("$name")
    fi
    command_handlers[$name]=$handler
    command_levels[$name]=$required_level
    command_help[$name]=$help_text
}

# Append an audit line. This is intentionally separate from SMS replies:
# command output may be sent back to the user, audit data stays local.
log_msg() {
    local message=$1
    local dir
    dir=$(dirname -- "$LOG_FILE")
    mkdir -p -- "$dir" 2>/dev/null || true
    printf '%s from=%s level=%s imsi=%s imei=%s request=%q %s\n' \
        "$(date '+%Y-%m-%d %H:%M:%S')" \
        "${incoming_number:-unknown}" \
        "${level:-unknown}" \
        "${imsi:-unknown}" \
        "${imei:-unknown}" \
        "$request" \
        "$message" >> "$LOG_FILE"
}

run_with_timeout() {
    /usr/bin/timeout --kill-after=5s "${COMMAND_TIMEOUT}s" "$@"
}

# Send a short reply to the SMS sender and also print it for daemon logs.
reply() {
    local text=$1
    if [[ -n "$incoming_number" && "$incoming_number" =~ ^\+[0-9]+$ ]]; then
        printf '%s' "$text" | run_with_timeout "$SMS_SEND" "$incoming_number" >/dev/null 2>&1 || true
    fi
    printf '%s\n' "$text"
}

# Normalize only whitespace. Case is preserved, so "reboot" and "Reboot"
# remain different commands.
normalize_request() {
    local text=$1
    text=${text//$'\r'/ }
    text=${text//$'\n'/ }
    # Trim and collapse whitespace. This is intentionally case-sensitive.
    read -r -a words <<< "$text"
    printf '%s' "${words[*]}"
}

require_level() {
    local required=$1
    if (( level < required )); then
        reply "Access denied"
        log_msg "denied required=$required"
        exit 1
    fi
}

require_no_args() {
    if (( $# != 0 )); then
        reply "Unexpected parameters: $*"
        log_msg "rejected reason=unexpected_parameters args=$(printf '%q' "$*")"
        return 1
    fi
}

format_command_error() {
    local rc=$1
    local title=$2
    local output=$3

    if (( rc == 124 || rc == 137 )); then
        reply "ERROR: timeout ${COMMAND_TIMEOUT}s: $title"
    elif [[ -n "$output" ]]; then
        reply "ERROR $rc: $output"
    else
        reply "ERROR $rc: $title"
    fi
}

# Run a configured backend command. Arguments are passed as argv, not through
# eval or sh -c, so SMS text cannot inject shell syntax.
run_backend() {
    local title=$1
    shift

    log_msg "run command=$title timeout=${COMMAND_TIMEOUT}s argv=$*"
    if [[ "$DRY_RUN" == "1" ]]; then
        reply "DRY RUN: $title"
        return 0
    fi

    local output rc
    output=$(run_with_timeout "$@" 2>&1)
    rc=$?
    log_msg "done command=$title rc=$rc output=$(printf '%q' "$output")"

    if (( rc == 0 )); then
        if [[ -n "$output" ]]; then
            reply "$output"
        else
            reply "OK: $title"
        fi
    else
        format_command_error "$rc" "$title" "$output"
    fi
    return "$rc"
}

run_command_quiet() {
    local title=$1
    shift

    log_msg "run command=$title timeout=${COMMAND_TIMEOUT}s argv=$*"
    if [[ "$DRY_RUN" == "1" ]]; then
        log_msg "dry_run command=$title"
        return 0
    fi

    local output rc
    output=$(run_with_timeout "$@" 2>&1)
    rc=$?
    log_msg "done command=$title rc=$rc output=$(printf '%q' "$output")"
    if (( rc != 0 )); then
        format_command_error "$rc" "$title" "$output"
    fi
    return "$rc"
}

load_command_modules() {
    if [[ ! -d "$COMMAND_DIR" ]]; then
        echo "Command directory not found: $COMMAND_DIR" >&2
        exit 1
    fi

    local module
    local found=0
    for module in "$COMMAND_DIR"/*.sh; do
        [[ -e "$module" ]] || continue
        found=1
        # shellcheck source=/dev/null
        source "$module"
    done

    if (( ! found )); then
        echo "No command modules found in $COMMAND_DIR" >&2
        exit 1
    fi
}

load_command_modules

# Validate sender before looking it up. Unknown and malformed senders do not
# reach command dispatch.
if ! [[ "$incoming_number" =~ ^\+[0-9]+$ ]]; then
    echo "Phone is not a regular number"
    log_msg "rejected reason=bad_phone"
    exit 1
fi

if [[ -z "${user_access[$incoming_number]+set}" ]]; then
    echo "Unknown number"
    log_msg "rejected reason=unknown_number"
    exit 1
fi

level=${user_access[$incoming_number]}
if (( level <= 0 )); then
    reply "Access is temporarily blocked"
    log_msg "rejected reason=blocked"
    exit 1
fi

request=$(normalize_request "$request")
if [[ -z "$request" ]]; then
    reply "Empty command"
    log_msg "rejected reason=empty_command"
    exit 1
fi

read -r -a request_words <<< "$request"
command_name=${request_words[0]}
command_args=("${request_words[@]:1}")

if [[ -z "${command_handlers[$command_name]+set}" ]]; then
    reply "Unknown command: $command_name"
    log_msg "rejected reason=unknown_command command=$command_name"
    exit 1
fi

required_level=${command_levels[$command_name]}
require_level "$required_level"

handler=${command_handlers[$command_name]}
"$handler" "${command_args[@]}"
