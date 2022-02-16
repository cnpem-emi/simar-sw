#!/bin/bash

SIMAR_FOLDER=/root/simar-software
RSYNC_ADDR=10.128.114.161

wpa_supplicant -B -P /root/simar-software/start/wpa-pid -u -s -c /etc/wpa_supplicant/wpa_supplicant.conf -i wlan0
PID=$(cat /root/simar-software/start/wpa-pid)
dhclient wlan0

sleep 10
rsync -a --delete-after 10.128.114.161::simar $SIMAR_FOLDER --contimeout=5
while ping -c 5 10.0.38.46 >> /dev/null; do
    sleep 10
done

kill $PID
