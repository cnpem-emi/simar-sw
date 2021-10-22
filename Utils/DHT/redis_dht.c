#include <hiredis/hiredis.h>
#include <stdio.h>
#include <unistd.h>

#include "../SPI/common.h"
#include "dht.h"

int main() {
  double hum, tem;
  int tries = 10;
  redisContext* c;
  struct timeval redis_timeout = {1, 500000};
  redisReply* reply;

  gpio_t pin = {.pin = P8_13};

  do {
    c = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
    if (c->err) {
      if (c->err == 1)
        printf(
            "Redis server instance not available. Have you initialized the "
            "Redis server? (Error code 1)\n");
      else
        printf("Unknown redis error (error code %d)\n", c->err);
      sleep(1);
    }
  } while (c->err);

  while (1) {
    tries = 10;
    while (tries) {
      if (bbb_dht_read(DHT11, pin, &hum, &tem) == 0) {
        printf("Temperature: %.2f\nHumidity: %.2f\n", tem, hum);
        reply = (redisReply*)redisCommand(c, "SET %s %.0f", "temperature", tem);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "SET %s %.0f", "humidity", hum);
        freeReplyObject(reply);
        break;
      } else {
        printf("Retrying... %d tries left\n", tries--);
        sleep(1);
      }
    }
    sleep(1);
  }
}
