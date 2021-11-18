#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bbb_dac.h"

int period;

int set_duty(int dut, char* pin) {
  char loc[44];
  snprintf(loc, 44, "%sduty_cycle", pin);

  int fd = open(loc, O_WRONLY);

  char duty[16];
  snprintf(duty, 16, "%d", period / 100 * dut);

  int ret = write(fd, duty, strlen(duty));
  close(fd);

  return ret;
}

int set_period(int per, char* pin) {
  char loc[44];
  snprintf(loc, 44, "%speriod", pin);

  int fd = open(loc, O_WRONLY);

  char period_s[16];
  snprintf(period_s, 16, "%d", per);

  period = per;

  int ret = write(fd, period_s, strlen(period_s));
  close(fd);

  return ret;
}

int enable(char* pin) {
  char loc[44];
  snprintf(loc, 44, "%senable", pin);

  int fd = open(loc, O_WRONLY);

  int ret = write(fd, "1", 2);
  close(fd);

  return ret;
}

int main(int argc, char* argv[]) {
  set_duty(2500, P8_13);
  enable(P8_13);
  set_period(20000, P8_13);

  for (int i = 0; i < 100; i += 1) {
    set_duty(i, P8_13);
    usleep(30000);
  }

  return 0;
}
