SHELL = /bin/sh
CFLAGS := -O2 -mtune=native -Wall
CC := gcc

COMPILE.c = $(CC) $(CFLAGS)

.PHONY: all dependencies ds lcd dac bdac badc adc init dht 


build: init_bme

init_volt: /usr/local/lib/libhiredis.so volt.c spiutil.o
	$(COMPILE.c) spiutil.o volt.c -lhiredis -lpthread -fno-trapping-math -o init_volt 

init_bme: /usr/local/lib/libhiredis.so bme_commons.o bme2.o bme.c i2cutil.o spiutil.o
	$(COMPILE.c) spiutil.o bme2.o bme_commons.o i2cutil.o bme.c -o init_bme -lhiredis

init_wireless: /usr/local/lib/libhiredis.so bme_commons.o bme2.o wireless.c i2cutil.o spiutil.o
	$(COMPILE.c) spiutil.o bme2.o bme_commons.o i2cutil.o wireless.c -o init_wireless -lhiredis -lpthread

single_bme: /usr/local/lib/libhiredis.so bme_commons.o bme2.o i2cutil.o spiutil.o BME280/single_bme.c
	$(COMPILE.c) spiutil.o bme2.o i2cutil.o bme_commons.o BME280/single_bme.c -o init_bme -lhiredis 

init_dht: /usr/local/lib/libhiredis.so dht.o
	$(COMPILE.c) spiutil.o dht.o ./DHT/redis_dht.c -o init_dht -lhiredis

spiutil.o: SPI/common.c
	$(COMPILE.c) -c SPI/common.c -o spiutil.o

i2cutil.o: I2C/common.c
	$(COMPILE.c) -c I2C/common.c -o i2cutil.o

bme2.o: BME280/bme2.c
	$(COMPILE.c) -c BME280/bme2.c

bmp2.o: BMP280/bmp2.c
	$(COMPILE.c) -c BMP280/bmp2.c

dht.o: DHT/dht.c
	$(COMPILE.c) -c DHT/dht.c

bme_commons.o: BME280/common/common.c
	$(COMPILE.c) -c BME280/common/common.c -o bme_commons.o

bmp_commons.o: BMP280/common/common.c
	$(COMPILE.c) -c BMP280/common/common.c -o bmp_commons.o

/usr/local/lib/libhiredis.so:
	git clone https://github.com/redis/hiredis.git
	cd ./hiredis; make && make install
	grep -qxF 'include /usr/local/lib' /etc/ld.so.conf || \
	echo 'include /usr/local/lib' >> /etc/ld.so.conf
	ldconfig
	rm -rf hiredis

examples: spiutil.o
	cd Utils; \
	$(COMPILE.c) ../spiutil.o ./ADC-DAC/BBB/bbb_dac.c -o bdac; \
	$(COMPILE.c) ../spiutil.o ./ADC-DAC/BBB/bbb_adc.c -o badc; \
	$(COMPILE.c) ../spiutil.o ./DHT/example.c ./DHT/dht.c -o dht; \
	$(COMPILE.c) ../spiutil.o ./LCD/example.c ./LCD/lcd.c -o lcd; \
	$(COMPILE.c) ./DS1820/ds.c -o ds; 

install:
	sed -i '/bind 127.0.0.1/c\bind 0.0.0.0' /etc/redis/*.conf
	sed -i '/protected-mode yes/c\protected-mode no' /etc/redis/*.conf
	cp ./Autostart/simar_sensors.service /etc/systemd/system/.
	cp ./Autostart/simar_log_conf /etc/logrotate.d/simar
	grep -qxF ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	systemctl daemon-reload
	systemctl start simar_sensors
	systemctl enable simar_sensors

install_volt:
	cp ./Autostart/simar_volt.service /etc/systemd/system/.
	service rsyslog restart
	systemctl daemon-reload
	systemctl start simar_volt
	systemctl enable simar_volt

install_wireless:
	sed -i -e '57c/root/simar-software/init_wireless' ./Autostart/simar_startup.sh -e '2cecho none > /sys/class/leds/beaglebone\:green\:usr3/trigger' ./Autostart/simar_startup.sh
	cp ./Autostart/simar_sensors.service /etc/systemd/system/.
	cp ./Autostart/simar_log_conf /etc/logrotate.d/simar
	cp ./Autostart/99-local.rules /etc/udev/rules.d/99-local.rules
	cp ./Autostart/usb-mount@.service /etc/systemd/system/usb-mount@.service
	cp ./Autostart/usb-mount.sh /usr/local/bin/usb-mount.sh
	grep -qxF ':syslogtag, isequal, "simar\:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	udevadm control --reload-rules
	systemctl daemon-reload
	systemctl start simar_sensors
	systemctl enable simar_sensors

clean:
	rm -f init* *dac *adc dht lcd ds *.o
