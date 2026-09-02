READ_TEMPERATURE_CMD=${READ_TEMPERATURE_CMD:-/usr/local/bin/read-temperature}
MAX_TEMPERATURE_SENSOR=${MAX_TEMPERATURE_SENSOR:-1000}
if ! [[ "$MAX_TEMPERATURE_SENSOR" =~ ^[0-9]+$ ]] || (( MAX_TEMPERATURE_SENSOR < 1 )); then
    MAX_TEMPERATURE_SENSOR=1000
fi

cmd_read_temperature_sensor() {
    local sensor=$1
    local sensor_number=${sensor#t}

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

register_command read cmd_read 1 'read t<N>'
