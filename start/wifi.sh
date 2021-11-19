#!/bin/bash
: '
if [[ $EUID -ne 0 ]]; then
   echo "🛑 This script must be run as root in order to enable the WPA supplicant services" 1>&2
   exit 1
fi

read -p "👤 Enter your Proteu authentication username: " USERNAME
read -s -p "🔒 Enter your password: " PASSWORD

HASH=$(echo -n ${PASSWORD} | iconv -t utf16le | openssl md4)
HASH=${HASH##* }
echo $HASH
'

cat > /etc/wpa_supplicant/wpa_supplicant.conf <<EOF
network={
  ssid="Devices"
  scan_ssid=1
  key_mgmt=NONE
}
EOF
