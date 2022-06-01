/*! @file leak.c
 * @brief Main starting point for fan RPM sensor module
 */

#include <hiredis/hiredis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

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

  char digital_buffer[1];

  uint32_t mode = 3;
  uint8_t bpw = 8;
  uint32_t speed = 1000000;

  int fd = spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

  const struct timespec* period = (const struct timespec[]){{1, 0}};

  int s;
  struct sockaddr_can addr;
  struct ifreq ifr;
  struct can_frame frame;

  if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
    syslog(LOG_ERR, "Could not open CAN socket");
    //return -2;
  }

  strcpy(ifr.ifr_name, "vcan0");
  ioctl(s, SIOCGIFINDEX, &ifr);

  memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    syslog(LOG_ERR, "CAN binding error");
    //return -2;
  }

  frame.can_id = 0x555;
  frame.can_dlc = 5;

  for (;;) {
    read_data(3, digital_buffer, 1);

    if (read(fd, digital_buffer, 1)) {
      for (int i = 0; i < 8; i++) {
        if (digital_buffer[0] >> i & 0b00000001) {
          /*snprintf(frame.data, 4, "%d %d", i, 1);  // TODO: Decide what to write
          if (write(s, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
            syslog(LOG_ERR, "CAN communication error");
            return -2;
          }
        }*/
        reply =
            redisCommand(c, "HSET leak_detector %d %d", i, ((digital_buffer[0] >> i) & 0b00000001));
        freeReplyObject(reply);
      }
    }

    nanosleep(period, NULL);
  }
}
