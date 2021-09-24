#!/bin/bash

echo start > /sys/class/remoteproc/remoteproc2/state
echo start > /sys/class/remoteproc/remoteproc1/state

config-pin P8_12 pruout
config-pin P9_27 pruout
config-pin P9_31 pruout
config-pin P9_29 pruout
config-pin P8_45 pruout
config-pin P8_46 pruout
config-pin P9_44 pruout
config-pin P8_12 pruout

config-pin P9_24 pruin
config-pin P9_28 pruin
config-pin P8_15 pruin
config-pin P9_30 pruin
config-pin P8_42 pruin
config-pin P8_41 pruin
config-pin P9_40 pruin
config-pin P8_15 pruin
