#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "spi/common.h"

#define AI_PIN "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

double get_rpm(double runtime, int* spinning) {
  char adc[5] = {0};

  int new_val = 0, old = 0;
  double valley_spacing = 0;
  double sum_valley_spacing = 0;
  double current_valley_spacing = 0;
  double count_valley_spacing = 0;
  int valley_count = 1;
  *spinning = 0;

  for (int i = 0; i < 100; i++) {
    int fd = open(AI_PIN, O_RDONLY);
    read(fd, adc, 4);
    close(fd);
    new_val = atoi(adc);
    if (abs(new_val - old) > 50 && old != 0) {
      *spinning = 1;
      if (new_val > 500)
        valley_count = 1;
      if (new_val < 200 && valley_count) {
        valley_spacing = current_valley_spacing;
        current_valley_spacing = 0;
        count_valley_spacing++;
        valley_count = 0;
        sum_valley_spacing += valley_spacing;
      }
    }
    current_valley_spacing++;
    old = new_val;
    usleep(1000);
  }

  return (60 / (sum_valley_spacing / count_valley_spacing * runtime)) / 3;
}

int main(int argc, char* argv[]) {
  clock_t t;

  int spinning = 0;

  t = clock();
  get_rpm(0.0013, &spinning);
  t = clock() - t;

  double runtime = ((double)t) / CLOCKS_PER_SEC / 18;

  get_rpm(runtime, &spinning);
}
