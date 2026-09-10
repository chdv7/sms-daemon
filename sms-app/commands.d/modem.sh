# Return modem identifiers passed by sms-daemon to this hook.
cmd_modem() {
    require_no_args "$@" || return 1
    reply "IMSI: ${imsi:-unknown}
IMEI: ${imei:-unknown}"
    log_msg "done command=modem"
}

register_command modem cmd_modem 2 modem
