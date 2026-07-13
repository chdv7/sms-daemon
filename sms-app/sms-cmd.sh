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

# Backend for parameterized "read t<N>".
# For "read t789" it is called as: $READ_TEMPERATURE_CMD t789
# VCC_CMD may be set to a command that prints supply voltage, for example:
#   VCC_CMD=/usr/local/bin/read-vcc
READ_TEMPERATURE_CMD=${READ_TEMPERATURE_CMD:-/usr/local/bin/read-temperature}
MAX_TEMPERATURE_SENSOR=${MAX_TEMPERATURE_SENSOR:-1000}
VCC_CMD=${VCC_CMD:-}
VCC_PATH=${VCC_PATH:-}
GPRS_CONNECT_TIMEOUT=${GPRS_CONNECT_TIMEOUT:-45}
if ! [[ "$MAX_TEMPERATURE_SENSOR" =~ ^[0-9]+$ ]] || (( MAX_TEMPERATURE_SENSOR < 1 )); then
    MAX_TEMPERATURE_SENSOR=1000
fi
if ! [[ "$GPRS_CONNECT_TIMEOUT" =~ ^[0-9]+$ ]] || (( GPRS_CONNECT_TIMEOUT < 1 )); then
    GPRS_CONNECT_TIMEOUT=45
fi

request=${1:-}
incoming_number=${2:-}
sent_time=${3:-}
receive_time=${4:-}
smsc=${5:-}
imsi=${6:-}
imei=${7:-}

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

run_command_quiet() {
    local title=$1
    shift

    log_msg "run command=$title argv=$*"
    if [[ "$DRY_RUN" == "1" ]]; then
        log_msg "dry_run command=$title"
        return 0
    fi

    local output rc
    output=$("$@" 2>&1)
    rc=$?
    log_msg "done command=$title rc=$rc output=$(printf '%q' "$output")"
    if (( rc != 0 )); then
        if [[ -n "$output" ]]; then
            reply "ERROR $rc: $output"
        else
            reply "ERROR $rc: $title"
        fi
    fi
    return "$rc"
}

wait_for_gprs_connection() {
    local deadline=$((SECONDS + GPRS_CONNECT_TIMEOUT))
    while (( SECONDS < deadline )); do
        if ip -4 -o addr show ppp0 2>/dev/null | grep -q 'inet '; then
            return 0
        fi
        sleep 1
    done
    return 1
}

cmd_pon_auto() {
    run_command_quiet "poff all" /usr/bin/poff -a || return 1
    sleep 3
    run_command_quiet "pon gprs" /usr/bin/pon gprs || return 1
    if [[ "$DRY_RUN" != "1" ]] && ! wait_for_gprs_connection; then
        reply "ERROR: gprs connection timeout"
        log_msg "failed command=pon_auto reason=gprs_timeout timeout=$GPRS_CONNECT_TIMEOUT"
        return 1
    fi
    run_command_quiet "pon vpn" /usr/bin/pon vpn || return 1
    reply "OK: pon auto"
    log_msg "done command=pon_auto"
}

cmd_pon() {
    require_level 2
    if (( $# != 1 )); then
        reply "Usage: pon gprs|vpn|auto"
        log_msg "rejected command=pon reason=bad_arg_count count=$#"
        return 1
    fi

    case "$1" in
        gprs|vpn)
            run_backend "pon $1" /usr/bin/pon "$1"
            ;;
        auto)
            cmd_pon_auto
            ;;
        *)
            reply "Unknown pon profile: $1"
            log_msg "rejected command=pon reason=unknown_profile profile=$1"
            return 1
            ;;
    esac
}

cmd_poff() {
    require_level 2
    if (( $# != 1 )); then
        reply "Usage: poff gprs|vpn|all"
        log_msg "rejected command=poff reason=bad_arg_count count=$#"
        return 1
    fi

    case "$1" in
        gprs|vpn)
            run_backend "poff $1" /usr/bin/poff "$1"
            ;;
        all)
            run_backend "poff all" /usr/bin/poff -a
            ;;
        *)
            reply "Unknown poff profile: $1"
            log_msg "rejected command=poff reason=unknown_profile profile=$1"
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

format_bytes_mb() {
    awk -v bytes="$1" 'BEGIN { printf "%.0f", bytes / 1024 / 1024 }'
}

format_bytes_gb() {
    awk -v bytes="$1" 'BEGIN { printf "%.1f", bytes / 1024 / 1024 / 1024 }'
}

read_temperature_value() {
    if [[ -r /sys/class/thermal/thermal_zone0/temp ]]; then
        awk '{ printf "%.1f C", $1 / 1000 }' /sys/class/thermal/thermal_zone0/temp
    else
        printf 'n/a'
    fi
}

read_vcc_value() {
    local value path label input

    if [[ -n "$VCC_CMD" ]]; then
        "$VCC_CMD" 2>/dev/null || printf 'n/a'
        return
    fi
    if [[ -n "$VCC_PATH" && -r "$VCC_PATH" ]]; then
        value=$(cat "$VCC_PATH")
        awk -v v="$value" 'BEGIN { if(v > 1000) printf "%.2f V", v / 1000000; else printf "%.2f V", v }'
        return
    fi
    if command -v vcgencmd >/dev/null 2>&1; then
        value=$(vcgencmd measure_volts 2>/dev/null | sed -n 's/^volt=//p')
        if [[ -n "$value" ]]; then
            printf '%s' "$value"
            return
        fi
    fi

    for path in /sys/class/power_supply/*/voltage_now; do
        [[ -r "$path" ]] || continue
        value=$(cat "$path")
        awk -v v="$value" 'BEGIN { printf "%.2f V", v / 1000000 }'
        return
    done

    for label in /sys/class/hwmon/hwmon*/in*_label; do
        [[ -r "$label" ]] || continue
        if grep -Eiq 'vcc|vin|5v|power|supply' "$label"; then
            input=${label%_label}_input
            if [[ -r "$input" ]]; then
                value=$(cat "$input")
                awk -v v="$value" 'BEGIN { printf "%.2f V", v / 1000 }'
                return
            fi
        fi
    done

    printf 'n/a'
}

read_cpu_percent() {
    local cpu user nice system idle iowait irq softirq steal guest guest_nice
    local idle1 total1 idle2 total2 diff_idle diff_total

    read -r cpu user nice system idle iowait irq softirq steal guest guest_nice < /proc/stat
    idle1=$((idle + iowait))
    total1=$((user + nice + system + idle + iowait + irq + softirq + steal))
    sleep 1
    read -r cpu user nice system idle iowait irq softirq steal guest guest_nice < /proc/stat
    idle2=$((idle + iowait))
    total2=$((user + nice + system + idle + iowait + irq + softirq + steal))
    diff_idle=$((idle2 - idle1))
    diff_total=$((total2 - total1))
    if (( diff_total <= 0 )); then
        printf 'n/a'
    else
        awk -v idle="$diff_idle" -v total="$diff_total" 'BEGIN { printf "%.0f%%", (100 * (total - idle)) / total }'
    fi
}

cmd_modem() {
    require_level 2
    require_no_args "$@" || return 1

    reply "IMSI: ${imsi:-unknown}
IMEI: ${imei:-unknown}"
    log_msg "done command=modem"
}

cmd_diag() {
    require_level 1
    require_no_args "$@" || return 1

    local mem_total_kb mem_avail_kb mem_used_mb mem_total_mb
    local disk_used_b disk_total_b disk_used_gb disk_total_gb
    local cpu temp vcc

    mem_total_kb=$(awk '/^MemTotal:/ { print $2 }' /proc/meminfo)
    mem_avail_kb=$(awk '/^MemAvailable:/ { print $2 }' /proc/meminfo)
    mem_used_mb=$(((mem_total_kb - mem_avail_kb) / 1024))
    mem_total_mb=$((mem_total_kb / 1024))

    read -r disk_total_b disk_used_b < <(df -B1 --output=size,used / | awk 'NR == 2 { print $1, $2 }')
    disk_used_gb=$(format_bytes_gb "$disk_used_b")
    disk_total_gb=$(format_bytes_gb "$disk_total_b")

    cpu=$(read_cpu_percent)
    temp=$(read_temperature_value)
    vcc=$(read_vcc_value)

    reply "mem: ${mem_used_mb}/${mem_total_mb} MB
disk: ${disk_used_gb}/${disk_total_gb} GB
cpu: ${cpu}
temp: ${temp}
vcc: ${vcc}"
    log_msg "done command=diag"
}

cmd_help() {
    require_level 1
    require_no_args "$@" || return 1

    local text
    text=$'Commands:
help
test
diag
read t<N>'
    if (( level >= 2 )); then
        text+=$'
modem
pon gprs
pon vpn
pon auto
poff gprs
poff vpn
poff all'
    fi
    if (( level >= 3 )); then
        text+=$'
reboot'
    fi
    reply "$text"
    log_msg "done command=help"
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
        reply "Usage: read t<N>"
        log_msg "rejected command=read reason=bad_arg_count count=$#"
        return 1
    fi

    case "$1" in
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
    poff)
        cmd_poff "${command_args[@]}"
        ;;
    read)
        cmd_read "${command_args[@]}"
        ;;
    diag)
        cmd_diag "${command_args[@]}"
        ;;
    modem)
        cmd_modem "${command_args[@]}"
        ;;
    help)
        cmd_help "${command_args[@]}"
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
