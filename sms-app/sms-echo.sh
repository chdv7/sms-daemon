#!/bin/bash

request="$1"
incoming_number="$2"

if ! [[ "$incoming_number" =~ ^\+[0-9]+$ ]]; then
    echo "Phone is not a regilar number"
    exit 1
fi

echo -n "Echo" "$request" | sms-send "$incoming_number"
