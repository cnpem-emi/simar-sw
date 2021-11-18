# SIMAR Software

[![Build](https://github.com/cnpem-iot/simar-software/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/cnpem-iot/simar-software/actions/workflows/c-cpp.yml)

SIMAR is an easily deployable intelligent platform for extensive monitoring and remote actuation of electrical systems and environment data, such as temperature, humidity, voltage, power factor and current. 

## Communication

Protocols that are confirmed working in BBB kernel version >=4.14 with the additional boards:
- [x] UART
- [x] I2C
- [x] SPI
- [x] OneWire
- [x] Analog (ADC/DAC)

Features that are only available in the TI kernel >=5.10:
- [x] PRU communication (used for glitch detection/power factor)

Works better with Debian 10 or later

# SIMAR sensors utility 

## Basic utilization

Run `./install.sh` and follow the onscreen instructions. Or:

### Temperature/Pressure/Humidity/Door
``` 
make init_bme
```

### Temperature/Pressure/Humidity (Plug-and-play, single sensor)
``` 
make init_wireless
```

### Wired connection
```
make install
``` 

### Wireless
```
make install_wireless
```

### Voltage/Current/Power Factor/Remote Actuation/Glitches/Frequency
*Requires kernel 5.10 or later*
```
make init_volt
make install_volt
```


## Important notes
- If SPI isn't working, check the bus before anything else. Depending on your board, the first bus might be either 1.0 or 0.0
- If OneWire fails to receive/transmit information, check your kernel version and `apt-get update && apt-get upgrade`

## Support
- Tested on AM3358
- PRU features only work on TI kernels (utilizes remoteproc)
