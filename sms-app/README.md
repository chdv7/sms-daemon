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
