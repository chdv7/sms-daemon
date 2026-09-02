# sample sms-daemon applications

SMS application is a program to be started on every SMS

To activate sms application use `sms_hook`  in your /etc/sms-daemon/config.cfg file (by default). 
Arguments are passed without shell expansion:

    argv[1] SMS text
    argv[2] sender number
    argv[3] sent timestamp from SMS/PDU
    argv[4] daemon receive timestamp
    argv[5] SMS center number
    argv[6] IMSI
    argv[7] IMEI

`sms-cmd.sh` reads IMSI and IMEI from these arguments, writes them to the audit log, and exposes them through the `modem` SMS command for active users.

Supported network control commands for supervisor/admin users:

    pon gprs
    pon vpn
    pon auto
    poff gprs
    poff vpn
    poff all

`pon gprs` and `pon vpn` call the matching `pon` profile. `poff gprs` and `poff vpn` call the matching `poff` profile. `poff all` calls `poff -a`. `pon auto` calls `poff -a`, waits 3 seconds, starts `pon gprs`, waits until `ppp0` gets an IPv4 address, and then starts `pon vpn`.

External commands are limited by `SMS_CMD_COMMAND_TIMEOUT`, default `45` seconds. `pon auto` also uses this value as the default `GPRS_CONNECT_TIMEOUT` while waiting for `ppp0` IPv4 address.

## Adding Commands

`sms-cmd.sh` is only a dispatcher. It loads every `*.sh` file from `commands.d` next to the script. After installation the default command directory is `/etc/sms-daemon/commands.d`. A command file must define a handler function and register it:

```bash
cmd_status() {
    require_no_args "$@" || return 1
    reply "OK"
    log_msg "done command=status"
}

register_command status cmd_status 1 status
```

`register_command` arguments are:

```text
register_command <sms-command> <handler-function> <minimum-access-level> <help-text>
```

Access levels are `1` for user, `2` for supervisor, and `3` for admin. The handler receives all words after the command name as its arguments.

The command directory can be overridden for tests or local customization:

```bash
SMS_CMD_COMMAND_DIR=/path/to/commands.d sms-cmd.sh 'status' '+19991111111'
```
