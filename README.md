# SIMAR Software

Protocols that are confirmed working in BBB kernel version >4.19 with the additional boards:
- [x] UART
- [x] I2C
- [x] SPI
- [x] OneWire
- [x] Analog (ADC/DAC)

Works much better with Redis >= 6.0.0

# SIMAR sensors utility 

## Basic utilization

### BME280
``` 
make init_bme
```

### BME280 (Plug-and-play, no door mode)
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

### Voltage/Current/Actuation
```
make init_volt
```

NOTE: This option requires user input

## Important notes
- If SPI isn't working, check the bus before anything else. Depending on your board, the first bus might be either 1.0 or 0.0
- If OneWire fails to receive/transmit information, check your kernel version and `apt-get update && apt-get upgrade`

## Support
- Tested on AM3358
- PRU features only work on TI kernels (utilizes remoteproc)
