// Quick program to read ADC through sysfs. Maxes out at ~400 Hz
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int adc_read(int fd) {
  char val[5];

  read(fd, &val, 5);
  return atoi(val);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: ./file <analog input (0 through 7)>\n");
    return -1;
  }
  char buf[49];

  snprintf(buf, 49, "/sys/bus/iio/devices/iio:device0/in_voltage%s_raw", argv[1]);

  int fd = open(buf, O_RDONLY);
  int val = adc_read(fd);

  printf("Value for pin: %d\n", val);
  close(fd);

  return 0;
}
