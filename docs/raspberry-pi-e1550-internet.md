# Raspberry Pi: Internet Through Huawei E1550

This guide describes a clean Raspberry Pi setup for mobile internet through a Huawei E1550 USB modem.

The tested layout is:

- `sms-daemon` uses `/dev/ttyUSB2` for SMS and USSD.
- PPP internet uses `/dev/ttyUSB0`.
- `/dev/ttyUSB1` is a QCDM diagnostic port and is not used for PPP.

## 1. Install Required Packages

On Raspberry Pi OS / Debian:

```bash
sudo apt update
sudo apt install ppp usb-modeswitch
```

`ppp` provides `pppd`, `chat`, `pon`, and `poff`.

## 2. Plug In The Modem And Check Ports

```bash
lsusb
ls -l /dev/ttyUSB*
```

For Huawei E1550 you normally see something like:

```text
/dev/ttyUSB0
/dev/ttyUSB1
/dev/ttyUSB2
```

If ModemManager is installed, it can show the modem port roles:

```bash
mmcli -L
mmcli -m 0
```

Expected roles for E1550 are usually:

```text
ttyUSB0 (at)
ttyUSB1 (qcdm)
ttyUSB2 (at)
```

If `sms-daemon` is already using `/dev/ttyUSB2`, keep PPP on `/dev/ttyUSB0`.

## 3. Choose APN

For the tested SIM, the modem registered as Tele2/t2 and worked with:

```text
internet.tele2.ru
```

Other common examples:

```text
internet.mts.ru
internet.beeline.ru
internet
```

Use the APN from the SIM/operator documentation. Wrong APN may still dial but then disconnect, redirect to an operator portal, or provide no real internet.

## 4. Automatic Typical Setup

From the project directory:

```bash
sudo scripts/setup-e1550-ppp.sh --apn internet.tele2.ru --port /dev/ttyUSB0
```

This creates:

```text
/etc/chatscripts/e1550
/etc/ppp/peers/e1550
/etc/systemd/system/e1550-ppp.service
```

It does not enable autostart by default.

To overwrite existing files:

```bash
sudo scripts/setup-e1550-ppp.sh --force --apn internet.tele2.ru --port /dev/ttyUSB0
```

To configure and start immediately:

```bash
sudo scripts/setup-e1550-ppp.sh --force --apn internet.tele2.ru --port /dev/ttyUSB0 --start
```

To also enable autostart at boot:

```bash
sudo scripts/setup-e1550-ppp.sh --force --apn internet.tele2.ru --port /dev/ttyUSB0 --enable
```

## 5. Manual Connection Control

Bring the connection up:

```bash
sudo pon e1550
```

Bring the connection down:

```bash
sudo poff e1550
```

Check whether `ppp0` appeared:

```bash
ip addr show ppp0
ip route
```

A working PPP link usually has an address like:

```text
ppp0: ...
    inet 100.x.x.x peer 10.64.64.64/32
```

Check HTTP/HTTPS through the modem explicitly:

```bash
curl --interface ppp0 -I https://yandex.ru
curl --interface ppp0 -I http://example.com
```

Do not rely only on `ping`: some mobile networks block ICMP.

## 6. Systemd Control

Start PPP:

```bash
sudo systemctl start e1550-ppp.service
```

Stop PPP:

```bash
sudo systemctl stop e1550-ppp.service
```

Show status:

```bash
sudo systemctl status e1550-ppp.service
```

Enable autostart:

```bash
sudo systemctl enable e1550-ppp.service
```

Disable autostart:

```bash
sudo systemctl disable e1550-ppp.service
```

Watch logs:

```bash
journalctl -u e1550-ppp.service -f
```

PPP/chat logs may also appear in the system journal:

```bash
journalctl -f | grep -E 'pppd|chat|ppp0'
```

## 7. Interaction With sms-daemon

Recommended split:

```text
sms-daemon: /dev/ttyUSB2
PPP data:   /dev/ttyUSB0
```

When PPP is active, `/dev/ttyUSB0` is no longer available for AT commands because the port carries the PPP data stream after `ATD*99#` returns `CONNECT`.

Do not use `/dev/ttyUSB1` for PPP on E1550. It is the QCDM diagnostic port.

## 8. Operator Captive Portal

If PPP connects, receives IP/DNS, but HTTP redirects to something like:

```text
http://access.rt.ru
https://access.rt.ru
```

then the modem setup is working, but the operator has restricted internet access. Typical reasons:

- SIM balance is empty.
- Data service is not active.
- The tariff requires portal activation.
- APN is not correct for the SIM.

In this state `ppp0` can exist and TCP may connect, but normal internet will not work until the operator restriction is removed.

## 9. Manual Files Created By The Script

Peer file example:

```text
/etc/ppp/peers/e1550
```

Chat script example:

```text
/etc/chatscripts/e1550
```

Systemd unit:

```text
/etc/systemd/system/e1550-ppp.service
```

Remove the setup:

```bash
sudo systemctl disable --now e1550-ppp.service
sudo rm -f /etc/systemd/system/e1550-ppp.service
sudo rm -f /etc/ppp/peers/e1550
sudo rm -f /etc/chatscripts/e1550
sudo systemctl daemon-reload
```
