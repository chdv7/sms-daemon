# sms-daemon
A small SMS and USSD sender/receiver.

Configuration is read from `sms-daemon.conf` by default:

    device=/dev/ttyUSB0

Use another file with `sms-daemon --config /path/to/sms-daemon.conf`.

Send an SMS body from stdin:

    sms-send +70123456789 ./outsms < message.txt

Send a USSD request:

    sms-send '*100#' ./outsms

USSD jobs are stored as `U"<encoded-payload>",<dcs>`. The daemon writes received SMS and USSD responses as XML files to `/tmp/sms-daemon/ReceivedSMS`.
