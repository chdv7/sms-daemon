# sample sms-daemon applications

SMS application is a program to be started on every SMS

To activate sms application use `sms_hook`  in your /etc/sms-daemon/config.cfg file (by default). 
Arguments are passed without shell expansion:

    argv[1] SMS text
    argv[2] sender number
    argv[3] sent timestamp from SMS/PDU
    argv[4] daemon receive timestamp
    argv[5] SMS center number
