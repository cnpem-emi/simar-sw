#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <syslog.h>
#include <unistd.h>

#include "SPI/common.h"

#define OUTLET_QUANTITY 8
#define RESOLUTION 0.01953125
#define PRU0_DEVICE_NAME "/dev/rpmsg_pru30"
#define PRU1_DEVICE_NAME "/dev/rpmsg_pru31"

const struct timespec* period = (const struct timespec[]){{1, 0}};

const char servers[11][16] = {
    "10.0.38.59",    "10.0.38.46",    "10.0.38.42",    "10.128.153.81",
    "10.128.153.82", "10.128.153.83", "10.128.153.84", "10.128.153.85",
    "10.128.153.86", "10.128.153.87", "10.128.153.88",
};

redisContext *c, *c_remote;
char name[72];
pthread_mutex_t spi_mutex;

void connect_remote() {
  int server_i = 0;
  int server_amount = sizeof(servers) / sizeof(servers[0]);

  do {
    c_remote = redisConnectWithTimeout(servers[server_i], 6379, (struct timeval){1, 500000});

    if (c_remote->err) {
      syslog(LOG_ERR, "%s remote Redis server not available, switching...\n", servers[server_i++]);

      nanosleep((const struct timespec[]){{1, 0}}, NULL);  // 1s
    }

    if (server_i == server_amount) {
      syslog(LOG_ERR, "No server found");
      exit(-3);
    }
  } while (c_remote->err);
}

void* command_listener() {
  redisReply *reply, *up_reply, *rb_reply;
  char names[8][64];
  uint8_t n_names = 0;
  up_reply = redisCommand(c, "LRANGE valid_sensors 0 -1");

  if (up_reply->type == REDIS_REPLY_ARRAY) {
    n_names = (int)up_reply->elements;
    for (int i = 0; i < n_names; i++)
      strncpy(names[i], up_reply->element[i]->str, 64);
  }

  freeReplyObject(up_reply);

  uint8_t command;

  connect_remote();
  syslog(LOG_NOTICE, "Redis command DB connected");

  while (1) {
    reply = redisCommand(c, "HMGET device ip_address name");

    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2)
      snprintf(name, 64, "SIMAR:%s:%s", reply->element[0]->str, reply->element[1]->str);

    freeReplyObject(reply);

    reply = redisCommand(c_remote, "HMGET %s 0 1 2 3 4 5 6 7", name);
    up_reply = redisCommand(c_remote, "HMGET %s:RB 0 1 2 3 4 5 6 7", name);

    select_module(0, 3);

    if (reply->type == REDIS_REPLY_ARRAY) {
      for (int i = 0; i < (int)reply->elements; i++) {
        command = reply->element[i]->str[0] - '0';
        if (reply->element[i]->str[0] != up_reply->element[i]->str[0]) {
          if (command != 1 && command != 0) {
            syslog(LOG_ERR, "Received malformed command: %d", command);
            rb_reply = redisCommand(c_remote, "HSET %s %d %d", name, i,
                                    up_reply->element[i]->str[0] - '0');
            freeReplyObject(rb_reply);
            continue;
          }
          pthread_mutex_lock(&spi_mutex);
          write_data(0b00000, command ? "\xff" : "\x00", 1);
          pthread_mutex_unlock(&spi_mutex);

          syslog(LOG_NOTICE, "User %s switched outlet %d %s", reply->element[i]->str + 2, i,
                 command == 1 ? "on" : "off");
          rb_reply = redisCommand(c_remote, "HSET %s:RB %d %d", name, i, command);
          freeReplyObject(rb_reply);
        }
      }
    } else if (reply->type == REDIS_REPLY_ERROR) {
      connect_remote();
    }

    freeReplyObject(reply);
    freeReplyObject(up_reply);

    reply = redisCommand(c_remote, "SMEMBERS %s:AT", name);

    if (reply->type == REDIS_REPLY_ARRAY) {
      for (int i = 0; i < (int)reply->elements; i++) {
        command = reply->element[i]->str[0] - '0';
        if (command < 8 && command < n_names) {
          rb_reply = redisCommand(c, "GET temperature_%s", names[command]);
          if (rb_reply->type == REDIS_REPLY_STRING) {
            pthread_mutex_lock(&spi_mutex);
            // TODO: Refine check logic after discussing standard
            write_data(0b00000, atof(rb_reply->str) > 26 ? "\x01" : "\x00", 1);
            pthread_mutex_unlock(&spi_mutex);
          }
          freeReplyObject(rb_reply);
        }
      }
    }

    select_module(0, 24);

    freeReplyObject(reply);
    nanosleep(period, NULL);
  }
}

void* pf_measure() {
  struct pollfd pollfd;
  uint8_t offset = 0;
  uint32_t counts[4];

  pollfd.fd = open(PRU1_DEVICE_NAME, O_RDWR);

  if (pollfd.fd < 0) {
    syslog(LOG_ERR, "Failed to communicate with PRU1");
    for (;;) {
    }
    // exit(-9);
  }

  char buf[512];

  for (;;) {
    write(pollfd.fd, "-", 1);

    if (read(pollfd.fd, buf, 512)) {
      for (int i = 0; i < 2; i++) {
        counts[i] = (buf[offset + 3] << 24) | (buf[offset + 2] << 16) | (buf[offset + 1] << 8) |
                    buf[offset];
        offset += 4;
      }

      // printf("%.3f",
      // (double)counts[0]/((double)counts[1]+(double)counts[0]));
    }
  }
}

void* glitch_counter() {
  struct pollfd prufd;

  prufd.fd = open(PRU0_DEVICE_NAME, O_RDWR);
  char buf[512];
  redisReply* reply;

  if (prufd.fd < 0) {
    syslog(LOG_ERR, "Failed to communicate with PRU0");
    for (;;) {
    }
    // exit(-9);
  }

  for (;;) {
    write(prufd.fd, "-", 1);
    usleep(999600);  // There is a 400 us offset, we're aiming for 1s
    write(prufd.fd, "-", 1);

    if (read(prufd.fd, buf, 512)) {
      reply = redisCommand(c, "SET glitch_count %d",
                           (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
      freeReplyObject(reply);
      read(prufd.fd, buf, 512);
    }
    nanosleep(period, NULL);
  }
}
double calc_voltage(char* buffer) {
  return (((buffer[1] & 0x0F) * 16) + ((buffer[0] & 0xF0) >> 4)) * RESOLUTION;
}

int main(int argc, char* argv[]) {
  openlog("simar_volt", 0, LOG_LOCAL0);
  redisReply* reply;

  syslog(LOG_NOTICE, "Starting up...");

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

  syslog(LOG_NOTICE, "Redis voltage DB connected");

  char buffer[3];
  double current = 0, voltage = 0, peak_voltage = 0;
  uint32_t mode = 1;
  uint8_t bpw = 16;
  uint32_t speed = 2000000;

  pthread_mutex_init(&spi_mutex, NULL);

  spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

  pthread_t cmd_thread;
  pthread_create(&cmd_thread, NULL, command_listener, NULL);

  pthread_t glitch_thread;
  pthread_create(&glitch_thread, NULL, glitch_counter, NULL);

  pthread_t pf_thread;
  pthread_create(&pf_thread, NULL, pf_measure, NULL);

  select_module(0, 24);

  // Dummy conversions
  spi_transfer("\x0F\x0F", buffer, 2);
  spi_transfer("\x0F\x0F", buffer, 2);

  spi_transfer("\x10\x83", buffer, 2);

  int i = 0;

  const struct timespec* inner_period = (const struct timespec[]){{0, 800000000L}};
  for (;;) {
    pthread_mutex_lock(&spi_mutex);
    peak_voltage = 0;

    // Current
    spi_transfer("\x10\x83", buffer, 2);
    spi_transfer("\x10\x83", buffer, 2);
    current = calc_voltage(buffer);
    current = (current - 2.5) / 0.125;

    // Throwaway
    spi_transfer("\x10\x9f", buffer, 2);

    for (i = 0; i < 240; i++) {
      spi_transfer("\x10\x9f", buffer, 2);
      voltage = calc_voltage(buffer);
      peak_voltage = voltage > peak_voltage ? voltage : peak_voltage;
    }  // Takes around 50 ms

    pthread_mutex_unlock(&spi_mutex);

    reply = redisCommand(c, "SET vch_%d %.3f", 0, peak_voltage * 31.5);
    freeReplyObject(reply);

    reply = redisCommand(c, "SET ich_%d %.3f", 0, current);
    freeReplyObject(reply);

    nanosleep(inner_period, NULL);
  }
}
