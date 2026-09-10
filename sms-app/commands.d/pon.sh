GPRS_CONNECT_TIMEOUT=${GPRS_CONNECT_TIMEOUT:-$COMMAND_TIMEOUT}
if ! [[ "$GPRS_CONNECT_TIMEOUT" =~ ^[0-9]+$ ]] || (( GPRS_CONNECT_TIMEOUT < 1 )); then
    GPRS_CONNECT_TIMEOUT=$COMMAND_TIMEOUT
fi

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
    if ! run_command_quiet "poff all" /usr/bin/poff -a; then
        log_msg "ignored command=pon_auto step=poff_all"
    fi
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

register_command pon cmd_pon 2 $'pon gprs\npon vpn\npon auto'
