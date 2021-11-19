SHELL = /bin/sh
CFLAGS := -O2 -mtune=native -Wall
CC := gcc

COMPILE.c = $(CC) $(CFLAGS)

SRCS = $(wildcard i2c/*.c spi/*.c bme280/*.c bme280/common/*.c)
PROGS = $(patsubst %.c,%,$(SRCS))

.PHONY: all

build: bme volt

volt: /usr/local/lib/libhiredis.so volt.c spi/common 
	$(COMPILE.c) $^ -lpthread -fno-trapping-math -o $@ -lhiredis

bme: /usr/local/lib/libhiredis.so $(PROGS)
	$(COMPILE.c) $^ bme.c -o $@ -lhiredis

wireless: /usr/local/lib/libhiredis.so $(PROGS)
	$(COMPILE.c) $^ wireless.c -o $@ -lpthread -lhiredis

%: %.c
	$(COMPILE.c) -c $< -o $@

/usr/local/lib/libhiredis.so:
	git clone https://github.com/redis/hiredis.git
	cd ./hiredis; make && make install
	grep -qxF 'include /usr/local/lib' /etc/ld.so.conf || \
	echo 'include /usr/local/lib' >> /etc/ld.so.conf
	ldconfig
	rm -rf hiredis

install:
	sed -i '/bind 127.0.0.1/c\bind 0.0.0.0' /etc/redis/*.conf
	sed -i '/protected-mode yes/c\protected-mode no' /etc/redis/*.conf
	cp ./start/simar_sensors.service /etc/systemd/system/.
	cp ./start/simar_log_conf /etc/logrotate.d/simar
	grep -qxF ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	systemctl daemon-reload
	systemctl start simar_sensors
	systemctl enable simar_sensors

install_volt:
	cp ./start/simar_volt.service /etc/systemd/system/.
	service rsyslog restart
	systemctl daemon-reload
	systemctl start simar_volt
	systemctl enable simar_volt

install_wireless:
	sed -i -e '57c/root/simar-software/wireless' ./start/simar_startup.sh -e '2cecho none > /sys/class/leds/beaglebone\:green\:usr3/trigger' ./start/simar_startup.sh
	cp ./start/simar_sensors.service /etc/systemd/system/.
	cp /etc/wpa_supplicant/ifupdown.sh /etc/ifplugd/action.d/ifupdown
	cp ./start/simar_log_conf /etc/logrotate.d/simar
	cp ./start/99-local.rules /etc/udev/rules.d/99-local.rules
	cp ./start/usb-mount@.service /etc/systemd/system/usb-mount@.service
	cp ./start/usb-mount.sh /usr/local/bin/usb-mount.sh
	grep -qxF ':syslogtag, isequal, "simar\:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	udevadm control --reload-rules
	systemctl daemon-reload
	systemctl start simar_sensors
	systemctl enable simar_sensors

clean:
	rm -f $(PROGS) init*
