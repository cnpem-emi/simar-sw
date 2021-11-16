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

config-pin P8_45 pruin
config-pin P8_46 pruout
config-pin P8_44 pruout
config-pin P8_43 pruin

#until [ -f /dev/spidev0.0 ]
#do
#     sleep 5
#done

echo Initializing power script...
/root/simar-software/init_volt
