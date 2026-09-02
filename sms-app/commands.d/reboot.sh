# Reboot is delayed so the SMS reply/logging path can finish first.
cmd_reboot() {
    require_no_args "$@" || return 1
    if [[ "$DRY_RUN" == "1" ]]; then
        reply "DRY RUN: reboot"
        return 0
    fi
    bash -c 'sleep 10 && systemctl reboot' &
    reply "OK: reboot"
    log_msg "done command=reboot"
}

register_command reboot cmd_reboot 3 reboot
