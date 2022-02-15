/*! @file leak.c
 * @brief Main starting point for fan RPM sensor module
 */

#include <hiredis/hiredis.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "../spi/common.h"

int main(int argc, char* argv[]) {
  openlog("simar", 0, LOG_LOCAL0);

  redisContext* c;
  redisReply* reply;

  do {
    c = redisConnectWithTimeout("127.0.0.1", 6379, (struct timeval){1, 500000});

    if (c->err) {
      if (c->err == 1)
        syslog(LOG_ERR,
               "Redis server instance not available. Have you "
               "initialized the Redis server? (Error code 1)\n");
      else
        syslog(LOG_ERR, "Unknown redis error (error code %d)\n", c->err);

      nanosleep((const struct timespec[]){{0, 700000000L}}, NULL);  // 700ms
    }
  } while (c->err);

  syslog(LOG_NOTICE, "Redis DB connected");

  char dummy_data[1] = "";
  char digital_buffer[1];

  uint32_t mode = 3;
  uint8_t bpw = 8;
  uint32_t speed = 1000000;

  int fd = spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

  const struct timespec* period = (const struct timespec[]){{1, 0}};

  while (1) {
    select_module(0, 2);
    spi_transfer(dummy_data, dummy_data, 1);
    select_module(0, 3);
    spi_transfer(dummy_data, dummy_data, 1);

    read(fd, digital_buffer, 1);
    for (int i = 0; i < 8; i++) {
      reply =
          redisCommand(c, "HSET leak_detector %d %d", i, ((digital_buffer[0] >> i) & 0b00000001));
      freeReplyObject(reply);
    }

    nanosleep(period, NULL);
  }
}
