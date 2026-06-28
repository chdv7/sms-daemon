#!/bin/bash
echo -n "Echo" "$1" | sms-send "$2"
