#!/bin/bash

# SPI0 - /dev/spidev0.#
config-pin p9_17 spi_cs
config-pin p9_18 spi
config-pin p9_21 spi
config-pin p9_22 spi_sclk

cp ./bin/pru*.out /lib/firmware/

echo pru0.out > /sys/class/remoteproc/remoteproc1/firmware
echo pru1.out > /sys/class/remoteproc/remoteproc2/firmware

echo start > /sys/class/remoteproc/remoteproc1/state
echo start > /sys/class/remoteproc/remoteproc2/state

config-pin P8_12 pruout
config-pin P9_27 pruout
config-pin P9_31 pruout
config-pin P9_29 pruout
config-pin P8_45 pruout
config-pin P8_46 pruout
config-pin P8_44 pruout
config-pin P8_43 pruout

config-pin P9_24 pruin
config-pin P9_28 pruin
config-pin P8_15 pruin
config-pin P9_30 pruin
config-pin P8_42 pruin
config-pin P8_41 pruin
config-pin P8_40 pruin
config-pin P8_39 pruin

# IO pins, just to make sure (p9_12 is reserved for w1)
echo out > /sys/class/gpio/gpio48/direction
echo out > /sys/class/gpio/gpio51/direction

# I2C pins
config-pin p9_19 i2c
config-pin p9_20 i2c

# Modprobes for OneWire
#modprobe wire
#modprobe w1-gpio

RETRIES=$(redis-cli get retries)
echo $RETRIES

if [[ $RETRIES -ge 20 ]]; then
    redis-cli set retries 0
    reboot
else
    redis-cli incr retries
fi

/root/simar-software/init_bme
