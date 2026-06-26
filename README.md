# sms-daemon
A small SMS and USSD sender/receiver.

Configuration is read from `/etc/sms-daemon/config.cfg` by default. The config file must exist; if a key is missing, the compiled default is used. Use another file with `-c`:

    sms-daemon -c /path/to/config.cfg
    sms-send -c /path/to/config.cfg '*100#'

Supported config keys:

    device=/dev/ttyUSB0
    job_dir=./outsms
    sms_dir=/tmp/sms-daemon/ReceivedSMS
    ussd_dir=/tmp/sms-daemon/ReceivedSMS
    log_file=/tmp/sms-daemon/Log
    debug=false

Send an SMS body from stdin:

    sms-send +70123456789 [./outsms] < message.txt

Send a USSD request:

    sms-send '*100#'

Send MMI/supplementary-service call-forwarding codes. Supported service codes are `21`, `67`, `61`, `62`, `002`, and `004`:

    sms-send '*#21#'              # query unconditional forwarding
    sms-send '**21*<number>#'     # register unconditional forwarding
    sms-send '##21#'              # erase unconditional forwarding
    sms-send '*#67#'              # query forwarding on busy
    sms-send '**61*<number>**20#' # register no-reply forwarding with timeout

If `sms-send` is called without an output directory, it uses `job_dir` from the config. USSD jobs are stored as `U"<encoded-payload>",<dcs>`. MMI/call-forwarding jobs are stored as `M<code>`. The daemon writes received SMS and USSD/MMI responses as XML files to `sms_dir` and `ussd_dir`.
