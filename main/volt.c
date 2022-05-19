/*! @file volt.c
 * @brief Main starting point for AC board module
 */

#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <syslog.h>
#include <unistd.h>

#include "../spi/common.h"

#define OUTLET_QUANTITY 8
#define RESOLUTION 0.01953125
#define VOLTAGE_CONST 68.8073472464
#define PRU0_DEVICE_NAME "/dev/rpmsg_pru30"
#define PRU1_DEVICE_NAME "/dev/rpmsg_pru31"

const char servers[11][16] = {
    "10.0.38.59",    "10.0.38.46",    "10.0.38.42",    "10.128.153.81",
    "10.128.153.82", "10.128.153.83", "10.128.153.84", "10.128.153.85",
    "10.128.153.86", "10.128.153.87", "10.128.153.88",
};

redisContext *c, *c_remote;
char name[72];
pthread_mutex_t spi_mutex;
double duty = 1;

/**
 * @brief Connects to a local Redis server (or exits, in case none are available)
 * @returns void
 */
void connect_local() {
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
}

/**
 * @brief Connects to remote Redis server (or exits, in case none are available)
 * @returns void
 */
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

/**
 * @brief Listens for commands sent to the Redis key on the main server
 * @returns void
 */
void* command_listener() {
  redisReply *reply, *up_reply, *rb_reply;
  char names[8][64];
  uint8_t delay_name = 21;
  uint8_t n_names = 0;
  uint8_t command;
  char msg_command[1] = {0x00};

  const struct timespec* period = (const struct timespec[]){{2, 0}};

  connect_remote();
  syslog(LOG_NOTICE, "Redis command DB connected");

  up_reply = redisCommand(c, "LRANGE valid_sensors 0 -1");

  if (up_reply->type == REDIS_REPLY_ARRAY) {
    n_names = (int)up_reply->elements;
    for (int i = 0; i < n_names; i++)
      strncpy(names[i], up_reply->element[i]->str, 64);
  }

  freeReplyObject(up_reply);

  reply = redisCommand(c, "HMGET device ip_address name");

  if (reply != NULL && reply->type == REDIS_REPLY_ARRAY)
    snprintf(name, 64, "SIMAR:%s:%s", reply->element[0]->str, reply->element[1]->str);
  else
    exit(-9);

  freeReplyObject(reply);

  up_reply = redisCommand(c_remote, "EXISTS %s", name);

  if (up_reply->type == REDIS_REPLY_INTEGER && up_reply->integer) {
    reply = redisCommand(c_remote, "HMGET %s 0 1 2 3 4 5 6 7", name);

    for (int i = 0; i < (int)reply->elements; i++) {
      if (reply->element[i]->str != NULL) {
        command = reply->element[i]->str[0] - '0';
        if (command != 1 && command != 0) {
          syslog(LOG_ERR, "Received malformed command: %d", command);
          rb_reply = redisCommand(c_remote, "HSET %s %d %d", name, i, 1);
          freeReplyObject(rb_reply);
          continue;
        }
        msg_command[0] += command << (i + 1);
      }
    }
    pthread_mutex_lock(&spi_mutex);
    write_data(15, msg_command, 1);
    pthread_mutex_unlock(&spi_mutex);
  } else {
    // Sets default values if they do not exist already
    reply = redisCommand(c_remote, "HSET %s 0 1 1 1 2 1 3 1 4 1 5 1 6 1", name);
  }
  freeReplyObject(up_reply);
  freeReplyObject(reply);

  while (1) {
    reply = redisCommand(c, "HMGET device ip_address name");

    if (delay_name > 20) {
      delay_name = 0;
      if (reply != NULL && reply->type == REDIS_REPLY_ARRAY)
        snprintf(name, 64, "SIMAR:%s:%s", reply->element[0]->str, reply->element[1]->str);
      else
        connect_remote();
    } else {
      delay_name++;
    }

    freeReplyObject(reply);

    reply = redisCommand(c_remote, "HMGET %s 0 1 2 3 4 5 6 7", name);
    up_reply = redisCommand(c_remote, "HMGET %s:RB 0 1 2 3 4 5 6 7", name);

    msg_command[0] = 0x00;

    if (reply->type == REDIS_REPLY_ARRAY) {
      for (int i = 0; i < (int)reply->elements; i++) {
        if (reply->element[i]->str != NULL) {
          command = reply->element[i]->str[0] - '0';

          if (command != 1 && command != 0) {
            syslog(LOG_ERR, "Received malformed command: %d", command);
            rb_reply = redisCommand(c_remote, "HSET %s %d %d", name, i,
                                    up_reply->element[i]->str[0] - '0');
            freeReplyObject(rb_reply);
            continue;
          }

          msg_command[0] += command << (i + 1);

          if (up_reply->element[i]->str == NULL ||
              reply->element[i]->str[0] != up_reply->element[i]->str[0]) {
            syslog(LOG_NOTICE, "User %s switched outlet %d %s", reply->element[i]->str + 2, i,
                   command == 1 ? "on" : "off");
            rb_reply = redisCommand(c_remote, "HSET %s:RB %d %d", name, i, command);
            freeReplyObject(rb_reply);
          }
        }
      }
      pthread_mutex_lock(&spi_mutex);
      write_data(15, msg_command, 1);
      pthread_mutex_unlock(&spi_mutex);
    } else if (reply->type == REDIS_REPLY_ERROR) {
      connect_remote();
    }

    freeReplyObject(reply);
    freeReplyObject(up_reply);

    nanosleep(period, NULL);
  }
}

/**
 * @brief Gets glitch count, frequency and duty cycle information from the PRU
 * @returns void
 */
void* glitch_counter() {
  struct pollfd prufd;
  const struct timespec* period = (const struct timespec[]){{0, 500000L}};

  prufd.fd = open(PRU1_DEVICE_NAME, O_RDWR);
  char buf[16];
  redisReply* reply;

  if (prufd.fd < 0) {
    syslog(LOG_ERR, "Failed to communicate with PRU1");
    exit(-9);
  }

  for (;;) {
    write(prufd.fd, "-", 1);
    usleep(4999600);
    write(prufd.fd, "-", 1);

    if (read(prufd.fd, buf, sizeof(buf))) {
      double duty_up = (buf[11] << 24) | (buf[10] << 16) | (buf[9] << 8) | buf[8];
      double duty_down = (buf[15] << 24) | (buf[14] << 16) | (buf[13] << 8) | buf[12];
      uint32_t frequency = (buf[7] << 24) | (buf[6] << 16) | (buf[5] << 8) | buf[4];
      duty = duty_up / (duty_up + duty_down);

      reply = redisCommand(c, "SET glitch %d",
                           (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
      freeReplyObject(reply);

      reply = redisCommand(c, "SET frequency %d", frequency / 5);
      freeReplyObject(reply);
      read(prufd.fd, buf, 16);
    }
    nanosleep(period, NULL);
  }
}

/**
 * @brief Calculate actual voltage, by picking out the 8 bits in the middle of the ADCs response
 * @param[in] buffer ADC response buffer
 * @returns Actual voltage value
 */
double calc_voltage(char* buffer) {
  return (((buffer[1] & 0x0F) * 16) + ((buffer[0] & 0xF0) >> 4)) * RESOLUTION;
}

int main(int argc, char* argv[]) {
  const struct timespec* inner_period = (const struct timespec[]){{1, 500000000L}};

  openlog("simar", 0, LOG_LOCAL0);
  redisReply* reply;

  syslog(LOG_NOTICE, "Starting up...");

  connect_local();

  syslog(LOG_NOTICE, "Redis voltage DB connected");

  char message[2] = {16, 0};
  char buffer[3];
  double current[7];
  double voltage = 0;
  uint32_t mode = 1;
  uint8_t bpw = 16;
  uint32_t speed = 200000;

  pthread_mutex_init(&spi_mutex, NULL);

  int spi_fd = spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

  pthread_t cmd_thread;
  pthread_create(&cmd_thread, NULL, command_listener, NULL);

  pthread_t glitch_thread;
  pthread_create(&glitch_thread, NULL, glitch_counter, NULL);

  syslog(LOG_NOTICE, "All threads initialized");

  // Dummy conversions
  pthread_mutex_lock(&spi_mutex);

  spi_transfer("\x0F\x0F", buffer, 2);
  spi_transfer("\x0F\x0F", buffer, 2);

  pthread_mutex_unlock(&spi_mutex);

  uint8_t i, read_fails = 0, low_current;
  struct timeval timeout = {5, 0};
  redisSetTimeout(c, timeout);

  syslog(LOG_NOTICE, "Main loop starting...");

  for (;;) {
    pthread_mutex_lock(&spi_mutex);
    transfer_module("\x01\x01", 2);

    // Current
    for (i = 1; i < 8; i++) {
      message[1] = 131 + i * 4;

      write(spi_fd, message, 2);
      read(spi_fd, buffer, 2);
      if (buffer[0] != 255 || buffer[1] != 255)
        current[i - 1] = (calc_voltage(buffer) - 2.5) / 0.66;
    }

    // Throwaway
    pthread_mutex_unlock(&spi_mutex);
    write(spi_fd, "\x10\x83", 2);
    read(spi_fd, buffer, 2);

    if (buffer[0] != 255 || buffer[1] != 255) {
      voltage = calc_voltage(buffer);
    } else {
      syslog(LOG_ERR, "Voltage reading failure");
      if (read_fails++ > 10)
        return (-2);
      continue;
    }

    if(voltage * VOLTAGE_CONST != 0.0) {
      reply = redisCommand(c, "SET volt %.3f", voltage * VOLTAGE_CONST);
      if (reply == NULL)
        connect_local();
      freeReplyObject(reply);
    }

    low_current = 1;

    for (i = 0; i < 7; i++) {
      if (current[i] > 100 || current[i] < -2)
        continue;
      reply = redisCommand(c, "HSET ich %d %.3f", 6 - i, current[i]);
      freeReplyObject(reply);

      if (current[i] > 0.8)
        low_current = 0;
    }

    reply = redisCommand(c, "SET pfactor %.3f", low_current ? 1.0 : duty);
    freeReplyObject(reply);

    nanosleep(inner_period, NULL);
  }
}
