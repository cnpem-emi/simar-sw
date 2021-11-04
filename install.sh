#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "🛑 This script must be run as root in order to install services." 1>&2
   exit 1
fi

:init

read -p "🌐 Is this installation wired? [y/n] [default: y] " I_TYPE

if [[ $I_TYPE = "y" ]] || [[ $I_TYPE = "" ]]
then
    read -p "Name your sensors (space separated): [optional]" SENSOR_NAMES 
    sed -i "s/\/root\/simar-software\/init_bme.*/\/root\/simar-software\/init_bme ${SENSOR_NAMES}/g" Autostart/simar_startup.sh
    make && make install
elif [[ $I_TYPE = "n" ]]
then
    ./Autostart/wifi.sh
else
    echo "🛑 Invalid option!"
    exit
fi
