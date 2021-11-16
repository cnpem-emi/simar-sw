#!/bin/bash

# IO pins, just to make sure (p9_12 is reserved for w1)
echo out > /sys/class/gpio/gpio48/direction
echo out > /sys/class/gpio/gpio51/direction

# I2C pins
config-pin p9_19 i2c
config-pin p9_20 i2c

RETRIES=$(redis-cli get retries)
echo $RETRIES

if [[ $RETRIES -ge 20 ]]; then
    redis-cli set retries 0
    reboot
else
    redis-cli incr retries
fi

echo Initializing BME script...
/root/simar-software/init_bme
