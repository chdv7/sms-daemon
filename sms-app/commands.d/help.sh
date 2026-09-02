cmd_help() {
    require_no_args "$@" || return 1

    local text='Commands:'
    local name
    for name in "${command_order[@]}"; do
        if (( level >= command_levels[$name] )); then
            text+=$'\n'
            text+=${command_help[$name]}
        fi
    done
    reply "$text"
    log_msg "done command=help"
}

register_command help cmd_help 1 help
