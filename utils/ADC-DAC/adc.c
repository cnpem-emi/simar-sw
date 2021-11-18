/// @file adc.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../SPI/common.h"

static uint32_t mode = 1;
static uint8_t bits = 8;
static uint32_t speed = 2000000;

void initialize() {
  gpio_t ADC_pin = {.pin = P9_24};

  spi_open("/dev/spidev0.0", &mode, &bits, &speed, &ADC_pin);
  char* rx;

  spi_transfer("\xff\xff", rx, 2);
  spi_transfer("\xff\xff", rx, 2);
  spi_transfer("\x83\x10", rx, 2);
}

void ch_int(int* data, char* st) {
  int length = strlen(st);
  printf("RX STRLEN: %d\n", length);
  for (int i = 0; i < length; i++) {
    data[i] = (int)st[i];
  }
}

int read_val(int channel) {
  char rx[2];
  char msg[3] = {131 + (channel * 4), 16};
  spi_transfer(msg, rx, 2);  // Not a typo
  spi_transfer(msg, rx, 2);

  int data[2] = {0, 0};
  ch_int(data, rx);

  return (data[0] & 0x0F) * 256 + data[1];
}

int main(int argc, char* argv[]) {
  if (geteuid() != 0) {
    printf("Please run this as root!");
    return -1;
  }

  int channel = argc > 1 ? atoi(argv[1]) : 0;
  initialize();

  int val = read_val(channel);
  printf("Val: %d %.2fV\n", val, (float)val / 4095.f * 5.f);

  spi_close();

  return 0;
}
