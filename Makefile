SHELL = /bin/sh
CFLAGS := -O2 -mtune=native -Wall
CC := gcc

COMPILE.c = $(CC) $(CFLAGS)

SRCS = $(wildcard i2c/*.c spi/*.c bme280/*.c bme280/common/*.c)
PROGS = $(patsubst %.c,%,$(SRCS))

KVER = $(shell uname -r)
KMAJ = $(shell echo $(KVER) | \
sed -e 's/^\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*.*/\1/')
KMIN = $(shell echo $(KVER) | \
sed -e 's/^[0-9][0-9]*\.\([0-9][0-9]*\)\.[0-9][0-9]*.*/\1/')

OUT = bin

.PHONY: all directories clean install_common

build: directories $(OUT)/fan $(OUT)/bme $(OUT)/volt $(OUT)/leak $(OUT)/pru1.out

directories: $(OUT)
wireless: $(OUT)/wireless

$(OUT):
	mkdir -p $(OUT)

$(OUT)/volt: /usr/local/lib/libhiredis.so main/volt.c spi/common 
	$(COMPILE.c) $^ -lpthread -fno-trapping-math -o $@ -lhiredis

$(OUT)/bme: /usr/local/lib/libhiredis.so main/bme.c utils/json/cJSON.o $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/wireless: /usr/local/lib/libhiredis.so main/wireless.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lpthread -lhiredis

$(OUT)/fan: /usr/local/lib/libhiredis.so main/fan.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/leak: /usr/local/lib/libhiredis.so main/leak.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/pru1.out:
	@if [ $(KMAJ) -gt 4 ] && [ $(KMIN) -gt 9 ] ; then \
		$(MAKE) -C pru ; \
    else \
    	echo "Kernel version incompatible with remoteproc implementation, skipping PRU..." ; \
	fi

%: %.c
	$(COMPILE.c) -c $^ -o $@

/usr/local/lib/libhiredis.so:
	git clone https://github.com/redis/hiredis.git
	cd ./hiredis; make && make install
	grep -qxF 'include /usr/local/lib' /etc/ld.so.conf || \
	echo 'include /usr/local/lib' >> /etc/ld.so.conf
	ldconfig
	rm -rf hiredis

utils/json/cJSON.o:
	gcc -c utils/json/cJSON.c -o $@

install_common:
	sed -i -e "3cSIMAR_FOLDER=$$PWD" ./start/simar_startup.sh
	cp ./start/services/simar@.service /etc/systemd/system/.
	cp ./start/conf/simar_log_conf /etc/logrotate.d/simar
	grep -qxF ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf

install: install_common
	sed -i '/bind 127.0.0.1/c\bind 0.0.0.0' /etc/redis/*.conf
	sed -i '/protected-mode yes/c\protected-mode no' /etc/redis/*.conf
	cp ./start/services/simar_volt.service /etc/systemd/system/.
	service rsyslog restart
	systemctl daemon-reload

install_wireless: install_common
	sed -i -e "8cExecStartPre=/bin/sleep 8 " ./start/services/simar@.service
	sed -i -e "9cExecStartPre=/bin/sh -c 'echo none > /sys/class/leds/beaglebone\:green\:usr3/trigger'" ./start/services/simar@.service
	sed -i -e "10cExecStartPre=/bin/sh -c 'echo none > /sys/class/leds/beaglebone\:green\:usr2/trigger'" ./start/services/simar@.service
	cp ./start/services/simar@.service /etc/systemd/system/.
	cp ./start/conf/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf
	cp ./start/conf/99-local.rules /etc/udev/rules.d/99-local.rules
	cp ./start/services/usb-mount@.service /etc/systemd/system/usb-mount@.service
	cp ./start/services/wpa_supplicant.service /etc/systemd/system/wpa_supplicant.service
	cp ./start/usb-mount.sh /usr/local/bin/usb-mount.sh
	grep -qxF ':syslogtag, isequal, "simar\:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	udevadm control --reload-rules
	systemctl daemon-reload
	systemctl start simar@wireless
	systemctl enable simar@wireless
	systemctl restart wpa_supplicant

clean:
	rm -rf $(PROGS) $(OUT)
