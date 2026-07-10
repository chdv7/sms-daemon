#!/usr/bin/env bash
set -euo pipefail

PROFILE=e1550
PORT=/dev/ttyUSB0
APN=internet.tele2.ru
BAUD=115200
ENABLE=0
START=0
FORCE=0

usage() {
    cat <<EOF
Usage: sudo $0 [options]

Configure PPP mobile internet for Huawei E1550 on Raspberry Pi.

Options:
  --profile NAME      PPP peer profile name. Default: e1550
  --port DEVICE       Modem data/AT port used for PPP. Default: /dev/ttyUSB0
  --apn APN           Mobile operator APN. Default: internet.tele2.ru
  --baud RATE         Serial speed. Default: 115200
  --enable            Enable systemd autostart for the PPP service
  --start             Start the PPP service after configuration
  --force             Overwrite existing config files without asking
  -h, --help          Show this help

Examples:
  sudo $0
  sudo $0 --apn internet.tele2.ru --port /dev/ttyUSB0 --start
  sudo $0 --profile e1550 --enable
EOF
}

while (($#)); do
    case "$1" in
        --profile)
            PROFILE=${2:?missing profile}; shift 2 ;;
        --port)
            PORT=${2:?missing port}; shift 2 ;;
        --apn)
            APN=${2:?missing apn}; shift 2 ;;
        --baud)
            BAUD=${2:?missing baud}; shift 2 ;;
        --enable)
            ENABLE=1; shift ;;
        --start)
            START=1; shift ;;
        --force)
            FORCE=1; shift ;;
        -h|--help)
            usage; exit 0 ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2 ;;
    esac
done

if [[ ${EUID} -ne 0 ]]; then
    echo "Run as root, for example: sudo $0" >&2
    exit 1
fi

if [[ ! "$BAUD" =~ ^[0-9]+$ ]]; then
    echo "Invalid baud rate: $BAUD" >&2
    exit 1
fi

CHAT_FILE=/etc/chatscripts/${PROFILE}
PEER_FILE=/etc/ppp/peers/${PROFILE}
SERVICE_FILE=/etc/systemd/system/${PROFILE}-ppp.service

check_overwrite() {
    local path=$1
    if [[ -e "$path" && "$FORCE" != "1" ]]; then
        echo "$path already exists. Use --force to overwrite." >&2
        exit 1
    fi
}

install_packages_hint() {
    local missing=0
    for cmd in /usr/sbin/pppd /usr/sbin/chat /usr/bin/pon /usr/bin/poff; do
        if [[ ! -x "$cmd" ]]; then
            missing=1
        fi
    done
    if ((missing)); then
        cat >&2 <<EOF
PPP tools are missing. Install them first:

  sudo apt update
  sudo apt install ppp
EOF
        exit 1
    fi
}

install_packages_hint
check_overwrite "$CHAT_FILE"
check_overwrite "$PEER_FILE"
check_overwrite "$SERVICE_FILE"

install -d -m 755 /etc/chatscripts /etc/ppp/peers /etc/systemd/system

cat > "$CHAT_FILE" <<EOF
ABORT BUSY
ABORT "NO CARRIER"
ABORT ERROR
ABORT "NO DIALTONE"
ABORT "Invalid Login"
ABORT "Login incorrect"
TIMEOUT 15
"" ATZ
OK AT+CGDCONT=1,"IP","${APN}"
OK ATD*99#
CONNECT ""
EOF
chmod 600 "$CHAT_FILE"

cat > "$PEER_FILE" <<EOF
${PORT}
${BAUD}
connect "/usr/sbin/chat -v -f ${CHAT_FILE}"
noauth
usepeerdns
noipdefault
ipcp-accept-local
ipcp-accept-remote
defaultroute
replacedefaultroute
hide-password
holdoff 10
maxfail 1
crtscts
modem
lock
EOF
chmod 600 "$PEER_FILE"

cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=Huawei E1550 PPP mobile internet (${PROFILE})
After=network.target sms-daemon.service

[Service]
Type=forking
ExecStart=/usr/bin/pon ${PROFILE}
ExecStop=/usr/bin/poff ${PROFILE}
RemainAfterExit=yes
Restart=on-failure
RestartSec=30

[Install]
WantedBy=multi-user.target
EOF
chmod 644 "$SERVICE_FILE"

systemctl daemon-reload

if ((ENABLE)); then
    systemctl enable "${PROFILE}-ppp.service"
fi

if ((START)); then
    systemctl start "${PROFILE}-ppp.service"
fi

cat <<EOF
Configured PPP profile: ${PROFILE}
  port:        ${PORT}
  apn:         ${APN}
  peer file:   ${PEER_FILE}
  chat file:   ${CHAT_FILE}
  systemd:     ${SERVICE_FILE}

Manual control:
  sudo pon ${PROFILE}
  sudo poff ${PROFILE}

Systemd control:
  sudo systemctl start ${PROFILE}-ppp.service
  sudo systemctl stop ${PROFILE}-ppp.service
  sudo systemctl status ${PROFILE}-ppp.service

Check connection:
  ip addr show ppp0
  ip route
  curl --interface ppp0 https://yandex.ru -I
EOF
