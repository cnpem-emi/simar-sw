#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include "../spi/common.h"

#define AI_PIN "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

double get_rpm(double runtime) {
  char adc[5] = {0};

  uint32_t new_val = 0, old = 0;
  double valley_spacing = 0;
  double sum_valley_spacing = 0;
  double current_valley_spacing = 0;
  double count_valley_spacing = 0;
  uint32_t valley_count = 1;

  for (uint8_t i = 0; i < 100; i++) {
    int fd = open(AI_PIN, O_RDONLY);
    read(fd, adc, 4);
    close(fd);
    new_val = atoi(adc);
    if (abs(new_val - old) > 50 && old != 0) {
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
  if (count_valley_spacing < 1)
    return 0.0;

  return (60 / (sum_valley_spacing / count_valley_spacing * runtime)) / 3;
}

int main(int argc, char* argv[]) {
  openlog("simar", 0, LOG_LOCAL0);
  
  clock_t t;
  redisContext* c;
  redisReply* reply;

  t = clock();
  get_rpm(0.0013);
  t = clock() - t;

  double runtime = ((double)t) / CLOCKS_PER_SEC / 18;

  do {
    c = redisConnectWithTimeout("127.0.0.1", 6379, (struct timeval){1, 500000});

    if (c->err) {
      if (c->err == 1)
        syslog(-2,
               "Redis server instance not available. Have you "
               "initialized the Redis server? (Error code 1)\n");
      else
        syslog(-2, "Unknown redis error (error code %d)\n", c->err);

      nanosleep((const struct timespec[]){{0, 700000000L}}, NULL);  // 700ms
    }
  } while (c->err);

  while (1) {
    reply = (redisReply*)redisCommand(c, "HSET fan speed %.3f", get_rpm(runtime));
    freeReplyObject(reply);
  }
}
