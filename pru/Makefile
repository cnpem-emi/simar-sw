PRU_SWPKG = /usr/lib/ti/pru-software-support-package

CC = clpru
LD = lnkpru
CFLAGS = --include_path=$(PRU_SWPKG)/include \
        --include_path=$(PRU_SWPKG)/include/am335x \
        -v3 -O4 --opt_for_speed=5 --endian=little \
        --define=am3359 --define=pru1 --hardware_mac=on --verbose
LDFLAGS = AM335x_PRU.cmd

SRCS = $(wildcard *.c)
PROGS = $(patsubst %.c,%,$(SRCS))

ASM_SRCS = $(wildcard *.asm)
ASM_PROGS = $(patsubst %.asm,%,$(ASM_SRCS))

OUT = ../bin

all: directories $(PROGS) $(ASM_PROGS) $(OUT)/pru1.out

directories: $(OUT)

%: %.c
	$(CC) $(CFLAGS) $< --output_file $@

%: %.asm
	$(CC) $(CFLAGS) $< --output_file $@

$(OUT)/pru1.out: $(ASM_PROGS) $(PROGS)
	$(LD) $(LDFLAGS) $^ --library=rpmsg_lib.lib -o $@

clean:
	rm -f pru1.out $(ASM_PROGS) $(PROGS)
