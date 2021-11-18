#!/bin/bash

wpa_supplicant -B -P /root/simar-software/start/wpa-pid -u -s -c /etc/wpa_supplicant/wpa_supplicant.conf -i wlan0
PID=$(cat /root/simar-software/start/wpa-pid)

while ping -c 5 10.0.38.46 >> /dev/null; do
    sleep 10
done

kill $PID