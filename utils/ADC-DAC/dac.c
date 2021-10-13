/// @file dac
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../SPI/common.h"

int fd;
static uint32_t mode = 1;
static uint8_t bits = 8;
static uint32_t speed = 2000000;

void initialize(int fd)
{
  char rx[3];
  gpio_t DAC_pin = { .pin = P9_23 };

  fd = spi_open("/dev/spidev0.0", &mode, &bits, &speed, &DAC_pin);

  spi_transfer("\x35\x96\x30", rx, 3);
  spi_transfer("\x20\x00\x00", rx, 3);
}

void write_val(int channel, int data)
{
  char msg[3] = {176 + channel, data >> 4, data << 4 & 0xff};

  spi_transfer(msg, msg, 3);
}

int main(int argc, char *argv[])
{
  if (geteuid() != 0)
  {
    printf("Please run this as root!");
    return -1;
  }

  if (argc < 3)
  {
    printf("Please use a valid value and channel!\n");
    return -1;
  }

  int channel = atoi(argv[1]);
  int data = atoi(argv[2]);

  initialize(fd);
  write_val(channel, data);

  spi_close();

  return 0;
}
