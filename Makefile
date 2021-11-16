SHELL = /bin/sh

.PHONY: all dependencies ds lcd dac bdac badc adc init dht 

build: init_bme

init_volt: /usr/local/lib/libhiredis.so volt.c spiutil.o
	gcc spiutil.o volt.c -o init_volt -lhiredis -lpthread -Wall -O3 -fno-trapping-math -mtune=native

init_bme: /usr/local/lib/libhiredis.so bme_commons.o bme2.o bme.c i2cutil.o spiutil.o
	gcc spiutil.o bme2.o bme_commons.o i2cutil.o bme.c -o init_bme -lhiredis -Wall -O2 -mtune=native

init_wireless: /usr/local/lib/libhiredis.so bme_commons.o bme2.o wireless.c i2cutil.o spiutil.o
	gcc spiutil.o bme2.o bme_commons.o i2cutil.o wireless.c -o init_wireless -lhiredis -Wall -lpthread -O3 -mtune=native

single_bme: /usr/local/lib/libhiredis.so bme_commons.o bme2.o i2cutil.o spiutil.o BME280/single_bme.c
	gcc spiutil.o bme2.o i2cutil.o bme_commons.o BME280/single_bme.c -o init_bme -lhiredis -Wall

init_dht: /usr/local/lib/libhiredis.so dht.o
	gcc spiutil.o dht.o ./DHT/redis_dht.c -o init_dht -lhiredis -Wall

spiutil.o: SPI/common.c
	gcc -c SPI/common.c -o spiutil.o -O2

i2cutil.o: I2C/common.c
	gcc -c I2C/common.c -o i2cutil.o -O2

bme2.o: BME280/bme2.c
	gcc -c BME280/bme2.c -O2

bmp2.o: BMP280/bmp2.c
	gcc -c BMP280/bmp2.c -O2

dht.o: DHT/dht.c
	gcc -c DHT/dht.c

bme_commons.o: BME280/common/common.c
	gcc -c BME280/common/common.c -o bme_commons.o -O2

bmp_commons.o: BMP280/common/common.c
	gcc -c BMP280/common/common.c -o bmp_commons.o -O2

/usr/local/lib/libhiredis.so:
	git clone https://github.com/redis/hiredis.git
	cd ./hiredis; make && make install
	grep -qxF 'include /usr/local/lib' /etc/ld.so.conf || \
	echo 'include /usr/local/lib' >> /etc/ld.so.conf
	ldconfig
	rm -rf hiredis

examples: spiutil.o
	cd Utils; \
	gcc ../spiutil.o ./ADC-DAC/BBB/bbb_dac.c -o bdac; \
	gcc ../spiutil.o ./ADC-DAC/BBB/bbb_adc.c -o badc; \
	gcc ../spiutil.o ./DHT/example.c ./DHT/dht.c -o dht; \
	gcc ../spiutil.o ./LCD/example.c ./LCD/lcd.c -o lcd; \
	gcc ./DS1820/ds.c -o ds; 

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
