#!/bin/bash

SIMAR_FOLDER=/root/simar-software
RSYNC_ADDR=10.128.114.161

rsync -a --delete-after $RSYNC_ADDR::simar $SIMAR_FOLDER --contimeout=5

# IO pins, just to make sure (p9_12 is reserved for w1)
echo out > /sys/class/gpio/gpio48/direction
echo out > /sys/class/gpio/gpio51/direction

# I2C pins
config-pin p9_19 i2c
config-pin p9_20 i2c

# SPI pins
config-pin p9_17 spi_cs
config-pin p9_18 spi
config-pin p9_21 spi
config-pin p9_22 spi_sclk

# PRU firmware
cp /root/simar-software/bin/pru*.out /lib/firmware/

# PRU pins
config-pin P8_45 pruin
config-pin P8_46 pruout
config-pin P8_44 pruout
config-pin P8_43 pruin
config-pin P8_41 pruin

: '
RETRIES=$(redis-cli get retries)
echo $RETRIES

if [[ $RETRIES -ge 20 ]]; then
    redis-cli set retries 0
    reboot
else
    redis-cli incr retries
fi
'

echo Initializing BME script...
systemctl start simar@bme
systemctl start simar@leak

if test -f "/root/simar-software/bin/pru1.out"; then
    systemctl start simar_volt
fi

