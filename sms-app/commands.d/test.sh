# Echo the original command text back to active users.
cmd_test_echo() {
    require_no_args "$@" || return 1
    reply "$request"
    log_msg "done command=test_echo"
}

register_command test cmd_test_echo 1 test
