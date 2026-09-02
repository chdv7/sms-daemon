cmd_poff() {
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

register_command poff cmd_poff 2 $'poff gprs\npoff vpn\npoff all'
