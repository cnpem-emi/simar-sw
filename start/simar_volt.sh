#!/bin/sh

cp /root/simar-software/bin/pru1.out /lib/firmware/pru1.out

#echo stop > /sys/class/remoteproc/remoteproc1/state
echo stop > /sys/class/remoteproc/remoteproc2/state

#echo pru0.out > /sys/class/remoteproc/remoteproc1/firmware
echo pru1.out > /sys/class/remoteproc/remoteproc2/firmware

#echo start > /sys/class/remoteproc/remoteproc1/state
echo start > /sys/class/remoteproc/remoteproc2/state

/root/simar-software/bin/volt
