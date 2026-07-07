#!/bin/bash
set -uo pipefail

# SMS command dispatcher.
# The first SMS word is the command name. Remaining words are parameters parsed
# by that command handler. SMS text is never executed as shell code.

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

# Backend for both "read temperature" and parameterized "read t<N>".
# For "read t789" it is called as: $READ_TEMPERATURE_CMD t789
READ_TEMPERATURE_CMD=${READ_TEMPERATURE_CMD:-/usr/local/bin/read-temperature}
MAX_TEMPERATURE_SENSOR=${MAX_TEMPERATURE_SENSOR:-1000}
if ! [[ "$MAX_TEMPERATURE_SENSOR" =~ ^[0-9]+$ ]] || (( MAX_TEMPERATURE_SENSOR < 1 )); then
    MAX_TEMPERATURE_SENSOR=1000
fi

request=${1:-}
incoming_number=${2:-}
sent_time=${3:-}
receive_time=${4:-}
smsc=${5:-}

# Append an audit line. This is intentionally separate from SMS replies:
# command output may be sent back to the user, audit data stays local.
log_msg() {
    local message=$1
    local dir
    dir=$(dirname -- "$LOG_FILE")
    mkdir -p -- "$dir" 2>/dev/null || true
    printf '%s from=%s level=%s request=%q %s\n' \
        "$(date '+%Y-%m-%d %H:%M:%S')" \
        "${incoming_number:-unknown}" \
        "${level:-unknown}" \
        "$request" \
        "$message" >> "$LOG_FILE"
}

# Send a short reply to the SMS sender and also print it for daemon logs.
reply() {
    local text=$1
    if [[ -n "$incoming_number" && "$incoming_number" =~ ^\+[0-9]+$ ]]; then
        printf '%s' "$text" | "$SMS_SEND" "$incoming_number" >/dev/null 2>&1 || true
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

# Run a configured backend command. Arguments are passed as argv, not through
# eval or sh -c, so SMS text cannot inject shell syntax.
run_backend() {
    local title=$1
    shift

    log_msg "run command=$title argv=$*"
    if [[ "$DRY_RUN" == "1" ]]; then
        reply "DRY RUN: $title"
        return 0
    fi

    local output rc
    output=$("$@" 2>&1)
    rc=$?
    log_msg "done command=$title rc=$rc output=$(printf '%q' "$output")"

    if (( rc == 0 )); then
        if [[ -n "$output" ]]; then
            reply "$output"
        else
            reply "OK: $title"
        fi
    else
        if [[ -n "$output" ]]; then
            reply "ERROR $rc: $output"
        else
            reply "ERROR $rc: $title"
        fi
    fi
    return "$rc"
}

# Command handlers below define the semantics of allowed SMS commands.
# Dispatch uses only the first SMS word as the command name. Every handler must
# validate its own parameters before calling a backend.
require_no_args() {
    if (( $# != 0 )); then
        reply "Unexpected parameters: $*"
        log_msg "rejected reason=unexpected_parameters args=$(printf '%q' "$*")"
        return 1
    fi
}

cmd_reboot() {
    require_level 3
    require_no_args "$@" || return 1
    run_backend "reboot" /sbin/reboot
}

cmd_pon() {
    require_level 2
    if (( $# != 1 )); then
        reply "Usage: pon gprs|pptp"
        log_msg "rejected command=pon reason=bad_arg_count count=$#"
        return 1
    fi

    case "$1" in
        gprs|pptp)
            run_backend "pon $1" /usr/bin/pon "$1"
            ;;
        *)
            reply "Unknown pon profile: $1"
            log_msg "rejected command=pon reason=unknown_profile profile=$1"
            return 1
            ;;
    esac
}

cmd_test_echo() {
    require_level 1
    require_no_args "$@" || return 1
    reply "$request"
    log_msg "done command=test_echo"
}

cmd_read_temperature() {
    require_level 1
    if [[ "$DRY_RUN" == "1" ]]; then
        reply "DRY RUN: read temperature"
        log_msg "dry_run command=read_temperature"
        return 0
    fi

    if [[ -x "$READ_TEMPERATURE_CMD" ]]; then
        run_backend "read temperature" "$READ_TEMPERATURE_CMD"
    elif [[ -r /sys/class/thermal/thermal_zone0/temp ]]; then
        local milli celsius
        milli=$(cat /sys/class/thermal/thermal_zone0/temp)
        celsius=$(awk -v t="$milli" 'BEGIN { printf "%.1f C", t / 1000 }')
        reply "Temperature: $celsius"
        log_msg "done command=read_temperature output=$(printf '%q' "$celsius")"
    else
        reply "Temperature sensor not found"
        log_msg "failed command=read_temperature reason=no_sensor"
        return 1
    fi
}

cmd_read_temperature_sensor() {
    local sensor=$1
    local sensor_number=${sensor#t}

    require_level 1
    if ! [[ "$sensor_number" =~ ^[0-9]+$ ]] || (( sensor_number < 1 || sensor_number > MAX_TEMPERATURE_SENSOR )); then
        reply "Invalid temperature sensor: $sensor"
        log_msg "rejected reason=bad_temperature_sensor sensor=$sensor"
        return 1
    fi

    if [[ "$DRY_RUN" == "1" ]]; then
        reply "DRY RUN: read $sensor"
        log_msg "dry_run command=read_temperature_sensor sensor=$sensor"
        return 0
    fi

    if [[ ! -x "$READ_TEMPERATURE_CMD" ]]; then
        reply "Temperature reader not found"
        log_msg "failed command=read_temperature_sensor reason=no_reader sensor=$sensor"
        return 1
    fi

    run_backend "read $sensor" "$READ_TEMPERATURE_CMD" "$sensor"
}

cmd_read() {
    require_level 1
    if (( $# != 1 )); then
        reply "Usage: read temperature|t<N>"
        log_msg "rejected command=read reason=bad_arg_count count=$#"
        return 1
    fi

    case "$1" in
        temperature)
            cmd_read_temperature
            ;;
        t[0-9]*)
            cmd_read_temperature_sensor "$1"
            ;;
        *)
            reply "Unknown read target: $1"
            log_msg "rejected command=read reason=unknown_target target=$1"
            return 1
            ;;
    esac
}

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

# Dispatch table. Only the first word is the command name; handlers parse and
# validate all remaining words.
case "$command_name" in
    reboot)
        cmd_reboot "${command_args[@]}"
        ;;
    pon)
        cmd_pon "${command_args[@]}"
        ;;
    read)
        cmd_read "${command_args[@]}"
        ;;
    test)
        cmd_test_echo "${command_args[@]}"
        ;;
    *)
        reply "Unknown command: $command_name"
        log_msg "rejected reason=unknown_command command=$command_name"
        exit 1
        ;;
esac
