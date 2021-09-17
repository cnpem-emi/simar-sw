#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root in order to enable the WPA supplicant services" 1>&2
   exit 1
fi

read -p "Enter your Proteu authentication username: " USERNAME
read -s -p "Enter your password: " PASSWORD

HASH=$(echo -n ${PASSWORD} | iconv -t utf16le | openssl md4)
HASH=${HASH##* }
echo $HASH

cat > /etc/wpa_supplicant/wpa_supplicant.conf <<EOF
network={
  ssid="Proteu"
  scan_ssid=1
  key_mgmt=WPA-EAP
  eap=PEAP
  identity="${USERNAME}"
  password=hash:${HASH}
  phase1="peaplabel=0"
  phase2="auth=MSCHAPV2"
}
EOF

cp wpa_supplicant.service /etc/systemd/system/wpa_supplicant.service
systemctl daemon-reload
systemctl start wpa_supplicant
systemctl enable wpa_supplicant

make -C .. && make -C .. install
