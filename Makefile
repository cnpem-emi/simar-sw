SHELL = /bin/sh
CFLAGS := -O2 -mtune=native -Wall
CC := gcc

COMPILE.c = $(CC) $(CFLAGS)

SRCS = $(wildcard i2c/*.c spi/*.c bme280/*.c bme280/common/*.c)
PROGS = $(patsubst %.c,%,$(SRCS))

OUT = bin

.PHONY: all directories clean install_common

build: directories $(OUT)/fan $(OUT)/bme $(OUT)/volt $(OUT)/leak $(OUT)/pru1.out

directories: $(OUT)
wireless: $(OUT)/wireless

$(OUT):
	mkdir -p $(OUT)

$(OUT)/volt: /usr/local/lib/libhiredis.so main/volt.c spi/common 
	$(COMPILE.c) $^ -lpthread -fno-trapping-math -o $@ -lhiredis

$(OUT)/bme: /usr/local/lib/libhiredis.so main/bme.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/wireless: /usr/local/lib/libhiredis.so main/wireless.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lpthread -lhiredis

$(OUT)/fan: /usr/local/lib/libhiredis.so main/fan.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/leak: /usr/local/lib/libhiredis.so main/leak.c $(PROGS)
	$(COMPILE.c) $^ -o $@ -lhiredis

$(OUT)/pru1.out:
	$(MAKE) -C pru

%: %.c
	$(COMPILE.c) -c $^ -o $@

/usr/local/lib/libhiredis.so:
	git clone https://github.com/redis/hiredis.git
	cd ./hiredis; make && make install
	grep -qxF 'include /usr/local/lib' /etc/ld.so.conf || \
	echo 'include /usr/local/lib' >> /etc/ld.so.conf
	ldconfig
	rm -rf hiredis

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
	sed -i -e "3cexport SIMAR_FOLDER=$$PWD" ./start/simar_startup.sh
	sed -i -e '44csimar@wireless' ./start/simar_startup.sh -e '2cecho none > /sys/class/leds/beaglebone\:green\:usr3/trigger' ./start/simar_startup.sh
	cp ./start/services/simar@.service /etc/systemd/system/.
	cp ./start/conf/wpa_supplicant.conf /etc/wpa_supplicant/wpa_supplicant.conf
	cp ./start/conf/99-local.rules /etc/udev/rules.d/99-local.rules
	cp ./start/services/usb-mount@.service /etc/systemd/system/usb-mount@.service
	cp ./start/usb-mount.sh /usr/local/bin/usb-mount.sh
	grep -qxF ':syslogtag, isequal, "simar\:" /var/log/simar/simar.log' /etc/rsyslog.conf || echo ':syslogtag, isequal, "simar:" /var/log/simar/simar.log' >> /etc/rsyslog.conf
	service rsyslog restart
	udevadm control --reload-rules
	systemctl daemon-reload
	systemctl start simar@wireless
	systemctl enable simar@wireless

clean:
	rm -rf $(PROGS) $(OUT)
