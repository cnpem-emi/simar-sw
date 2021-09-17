# DS1820

Add the following lines to `/boot/uEnv.txt`:

```
uboot_overlay_addr0=/lib/firmware/BB-W1-P9.12-00A0.dtbo
```

Preferrably, you should update the kernel and all available tools, as new capes/DTOs/fixes are released often. You can do that with:

```
apt-get update
apt-get upgrade
cd /opt/scripts/tools/
git pull
./update_kernel.sh
reboot
``` 

As of now, any tools/kernel version made after July 2019 should work.

Reboot, and then execute:

```
modprobe wire
modprobe w1-gpio
```

To check if it worked, cat `/sys/bus/w1/devices/your_ds1820_device/w1_slave`


## Performance

- Memory usage is constant (<1MB)
- No memory leaks detected
- CPU usage peaks at 1.3% on every measurement
- Due to FS access limitations, poll rate is limited to ~1-2 Hz
