/*! @file bme.c
 * @brief Main starting point for BME280 sensor module
 */

#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "../bme280/common/common.h"
#include "../sht3x/sht3x.h"
#include "../utils/json/cJSON.h"

// Set to 3 to enable the I2C Expansion Board
#define ERROR_THRESHOLD 5
#define EXT_BOARD_I2C_LEN 6

uint8_t iface_board_len = 4;

const char servers[3][32] = {"10.0.38.46", "10.0.38.42", "10.0.38.59"};

/**
 * @brief Updates door opening status
 *
 * @param[in] sensor : Pointer to sensor
 *
 * @details Considering the closed server racks work under negative pressure, a
 * sustained "significant" rise in pressure may indicate a door opening.
 *
 * However, a change in temperature may (albeit slowly) alter the pressure,
 * so the best solution is to maintain a moving open/closed pressure average
 * and listen for sudden changes.
 *
 * @return void
 */
void get_open_iter(struct bme_sensor_data* sensor) {
  // 0.23 refers to the standard deviation of the population over a period of 3 minutes
  if (sensor->average - sensor->data.pressure < -0.23 ||
      (sensor->open_average != 0 && sensor->open_average - sensor->data.pressure < 0.23)) {
    if (sensor->strikes_closed > (WINDOW_SIZE - 1))
      sensor->is_open = 1;
    else
      sensor->strikes_closed++;
  } else {
    if (sensor->strikes_closed < 1)
      sensor->is_open = sensor->open_average = 0;
    else
      sensor->strikes_closed--;
  }
}

/**
 * @brief Updates moving average, door opening status
 *
 * @param[in] sensor : Pointer to sensor
 *
 * @details The "average" pressure is important to determine sudden changes, however,
 * it'll only respond to gradual changes in order to deter statistical
 * abnormalities.
 *
 * As for the "open" average, it'll get reset every time the door is closed,
 * which is the default state.
 *
 * The closed door pressure moving average ignores sudden large pressure
 * increases, but is more sensitive to pressure decreases. The opposite happens
 * to the open door pressure moving average.
 *
 * @return void
 */
void update_open(struct bme_sensor_data* sensor) {
  // Moves moving average window 1 position to the left to accomodate new value
  double cache[WINDOW_SIZE];
  for (int i = 0; i < WINDOW_SIZE; i++)
    cache[i] = sensor->window[i];

  double sum = 0;

  double diff = sensor->average - sensor->data.pressure;
  double open_diff = sensor->open_average - sensor->data.pressure;

  if ((sensor->is_open && sensor->strikes_closed == WINDOW_SIZE) ||
      (!sensor->is_open && sensor->strikes_closed == 0)) {
    if (sensor->average == 0 || (diff > -0.08 && diff < 0.1 && !sensor->is_open)) {
      for (int i = WINDOW_SIZE - 1; i > 0; i--)
        sensor->window[i - 1] = cache[i];
      sensor->window[WINDOW_SIZE - 1] = sensor->data.pressure;
      for (int i = 0; i < WINDOW_SIZE; i++)
        sum += sensor->window[i];
      sensor->average = sum / WINDOW_SIZE;
    } else if (sensor->is_open &&
               (sensor->open_average == 0 || (open_diff > -0.1 && open_diff < 0.08))) {
      for (int i = WINDOW_SIZE - 1; i > 0; i--)
        sensor->window[i - 1] = cache[i];
      sensor->window[WINDOW_SIZE - 1] = sensor->data.pressure;
      for (int i = 0; i < WINDOW_SIZE; i++)
        sum += sensor->window[i];
      sensor->open_average = sum / WINDOW_SIZE;
    }
  }

  uint8_t past_open = sensor->is_open;

  get_open_iter(sensor);

  if (past_open != sensor->is_open) {
    for (int i = 0; i < WINDOW_SIZE; i++) {
      sensor->window[i] = sensor->data.pressure;
    }
  }
}

int main(int argc, char* argv[]) {
  openlog("simar", 0, LOG_LOCAL0);

  redisContext *c, *c_remote;
  redisReply *reply, *reply_remote;

  struct bme_sensor_data bme_sensors[16];
  struct sht3x_sensor_data sht_sensors[16];

  bme_sensors[0].dev.settings.osr_h = BME280_OVERSAMPLING_4X;
  bme_sensors[0].dev.settings.osr_p = BME280_OVERSAMPLING_16X;
  bme_sensors[0].dev.settings.osr_t = BME280_OVERSAMPLING_4X;
  bme_sensors[0].dev.settings.filter = BME280_FILTER_COEFF_OFF;

  uint8_t valid_bme = 0;
  uint8_t valid_sht = 0;
  uint8_t board_addr;

  int fd = open("/opt/device.json", O_RDONLY);
  if (fd > 0) {
    int len = lseek(fd, 0, SEEK_END);
    char* json_buf = malloc(len + 1);
    lseek(fd, 0, SEEK_SET);
    read(fd, json_buf, len);

    const cJSON* boards = NULL;
    const cJSON* board = NULL;
    cJSON* boards_json = cJSON_Parse(json_buf);

    if (json_buf != NULL) {
      boards = cJSON_GetObjectItemCaseSensitive(boards_json, "boards");
      cJSON_ArrayForEach(board, boards) {
        cJSON* address = cJSON_GetObjectItemCaseSensitive(board, "type");
        if (cJSON_IsString(address) && strcmp(address->valuestring, "spiExpansion") == 0) {
          cJSON* board_no = cJSON_GetObjectItemCaseSensitive(board, "address");
          if (cJSON_IsNumber(board_no)) {
            board_addr = board_no->valueint;
            set_ext_addr(board_addr);
            iface_board_len = 3;
          }
        }
      }
    }

    cJSON_Delete(boards_json);
    free(json_buf);
  }

  for (int i = 0; i < iface_board_len * 2; i++) {
    struct bme_sensor_data sensor;
    sensor.dev = bme_sensors[0].dev;
    sensor.id.mux_id = i % iface_board_len;
    sensor.id.ext_mux_id = -1;

    uint8_t sensor_addr = i < iface_board_len ? BME280_I2C_ADDR_PRIM : BME280_I2C_ADDR_SEC;
    uint8_t sensor_status = bme_init(&sensor.dev, &sensor.id, sensor_addr);

    if (sensor_status == BUS_FAIL)
      return BUS_FAIL;

    if (sensor_status == BME280_OK) {
      bme_sensors[valid_bme] = sensor;
      bme_sensors[valid_bme].dev.intf_ptr = &bme_sensors[valid_bme].id;
      bme_sensors[valid_bme].average = bme_sensors[valid_bme].open_average = 0;
      bme_sensors[valid_bme].strikes_closed = bme_sensors[valid_bme].is_open = 0;

      snprintf(bme_sensors[valid_bme].name, MAX_NAME_LEN, "sensor_%d_%x", i % iface_board_len,
               sensor_addr);

      syslog(LOG_INFO, "Initialized BMx device with address 0x%x at channel %d", sensor_addr,
             sensor.id.mux_id);
      valid_bme++;
    } else {
      struct sht3x_sensor_data sht_sensor = {.id.mux_id = i % iface_board_len, .id.ext_mux_id = -1};
      uint8_t sht_sensor_addr = i < iface_board_len ? SHT3X_I2C_ADDR_DFLT : SHT3X_I2C_ADDR_ALT;

      if (sht3x_init(&sht_sensor, sht_sensor_addr) == BME280_OK) {
        sht_sensors[valid_sht] = sht_sensor;
        snprintf(sht_sensors[valid_sht].name, MAX_NAME_LEN, "sensor_%d_%x", i % iface_board_len,
                 sht_sensor_addr);

        syslog(LOG_INFO, "Initialized SHT3x device with address 0x%x at channel %d",
               sht_sensor_addr, sht_sensor.id.mux_id);
        valid_sht++;
      }
    }
  }

  if (iface_board_len == 3) {
    uint32_t mode = 3;
    uint8_t bpw = 8;
    uint32_t speed = 1000000;

    spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

    for (int i = 1; i < EXT_BOARD_I2C_LEN + 2; i++) {
      if (i % 4 == 0)
        continue;

      struct bme_sensor_data sensor;
      sensor.dev = bme_sensors[0].dev;
      sensor.id.mux_id = 3;

      /* Gets multiplexer channel ID for I2C extension board.
       *  Up to the fourth channel, only the first mux is used, which
       *  is selected by the first pair of bits (from LSB).
       *  From the fourth channel onwards, the second mux. is used.
       *  Channels xx00 and 00xx cannot be used, as they are currently
       *  used for "parking" each multiplexer to prevent cross-communication.
       */
      sensor.id.ext_mux_id = i < 4 ? i % 4 : (i % 4) << 2;

      uint8_t sensor_addr = BME280_I2C_ADDR_PRIM;
      uint8_t sht_sensor_addr = SHT3X_I2C_ADDR_DFLT;

      do {
        uint8_t sensor_status = bme_init(&sensor.dev, &sensor.id, sensor_addr);

        if (sensor_status == BUS_FAIL)
          return BUS_FAIL;

        if (sensor_status == BME280_OK) {
          bme_sensors[valid_bme] = sensor;
          bme_sensors[valid_bme].dev.intf_ptr = &bme_sensors[valid_bme].id;
          bme_sensors[valid_bme].average = bme_sensors[valid_bme].open_average = 0;
          bme_sensors[valid_bme].strikes_closed = bme_sensors[valid_bme].is_open = 0;

          snprintf(bme_sensors[valid_bme].name, MAX_NAME_LEN, "sensor_%d_%x",
                   i + iface_board_len + 1, sensor_addr);

          syslog(LOG_INFO,
                 "Initialized BMx device on expansion board with address 0x%x at channel %d",
                 sensor_addr, sensor.id.ext_mux_id);
          valid_bme++;
        } else {
          struct sht3x_sensor_data sht_sensor = {.id.mux_id = i % iface_board_len,
                                                 .id.ext_mux_id = -1};
          if (sht3x_init(&sht_sensor, sht_sensor_addr) == BME280_OK) {
            sht_sensors[valid_sht] = sht_sensor;
            snprintf(sht_sensors[valid_sht].name, MAX_NAME_LEN, "sensor_%d_%x", i % iface_board_len,
                     sht_sensor_addr);

            syslog(LOG_INFO,
                   "Initialized SHT3x on expansion board device with address 0x%x at channel %d",
                   sht_sensor_addr, sht_sensor.id.ext_mux_id);
            valid_sht++;
          }
        }
      } while (sensor_addr++ < BME280_I2C_ADDR_SEC && sht_sensor_addr++ < SHT3X_I2C_ADDR_ALT);
    }

    unselect_i2c_extender();
  }

  if (valid_bme < 1 && valid_sht < 1) {
    syslog(LOG_CRIT, "No sensors found");
    return SENSOR_FAIL;
  }

  syslog(LOG_NOTICE, "Starting up...");

  int server_i = 0;

  do {
    c = redisConnectWithTimeout("127.0.0.1", 6379, (struct timeval){1, 500000});
    c_remote = redisConnectWithTimeout(servers[server_i], 6379, (struct timeval){1, 500000});

    if (c->err) {
      if (c->err == 1)
        syslog(LOG_ERR,
               "Redis server instance not available. Have you "
               "initialized the Redis server? (Error code 1)\n");
      else
        syslog(LOG_ERR, "Unknown redis error (error code %d)\n", c->err);

      nanosleep((const struct timespec[]){{0, 700000000L}}, NULL);  // 700ms
    }

    if (c_remote->err) {
      if (server_i == 0) {
        syslog(LOG_ERR,
               "No remote Redis server instance for calibration is available. "
               "Attempting to fetch local mirror.\n");
        c_remote = c;
      }

      syslog(LOG_ERR, "%s remote Redis server not available, switching...\n", servers[server_i++]);
    }

  } while (c->err || c_remote->err);

  syslog(LOG_NOTICE, "Redis DB connected");
  int retries = 0;

  // Populate moving average window before anything else

  double pressure_delta = 0;

  reply_remote = redisCommand(c_remote, "GET wgen2_pressure");

  if (reply_remote->str) {
    double external_pressure = atof(reply_remote->str);
    reply = redisCommand(c, "GET last_ext_pressure");

    if (reply->str) {
      double pressure_cache = atof(reply->str);
      pressure_delta = external_pressure - pressure_cache;
      syslog(LOG_NOTICE,
             "Deviation detected, pressure delta is %.3f, current external "
             "pressure is %.3f and last recorded pressure is %.3f\n",
             pressure_delta, external_pressure, pressure_cache);
    }
    freeReplyObject(reply);
  }

  freeReplyObject(reply_remote);

  reply = (redisReply*)redisCommand(c, "DEL valid_sensors");
  freeReplyObject(reply);

  for (int i = 0; i < valid_bme; i++) {
    reply = redisCommand(c, "HGET %s avg", bme_sensors[i].name);

    if (!reply->str) {
      bme_sensors[i].past_pres = 0;

      // First 3 readouts are discarded
      for (int k = 0; k < 3; k++) {
        bme_read(&bme_sensors[i].dev, &bme_sensors[i].data);
        nanosleep((const struct timespec[]){{0, 250000000L}}, NULL);
      }

      for (int j = 0; j < WINDOW_SIZE; j++) {
        nanosleep((const struct timespec[]){{0, 250000000L}}, NULL);
        if(bme_read(&bme_sensors[i].dev, &bme_sensors[i].data) != BME280_OK) {
          continue;
        }

        if (bme_sensors[i].data.pressure > 850 && bme_sensors[i].data.pressure < 1000) {
          bme_sensors[i].window[j] = bme_sensors[i].past_pres = bme_sensors[i].data.pressure;
        } else {
          --j;
          ++retries;
          if (retries > 10) {
            syslog(LOG_ERR, "Could not obtain realistic data from sensor number %d\n", i);
            return SENSOR_FAIL;
          }
        }
      }
    } else {
      double avg = atof(reply->str);

      syslog(LOG_NOTICE, "Pressure moving average for %d was %.3f\n", i, avg);

      avg += pressure_delta;
      bme_sensors[i].past_pres = avg;

      while (1) {
        bme_read(&bme_sensors[i].dev, &bme_sensors[i].data);
        if (bme_sensors[i].data.pressure > 900 && bme_sensors[i].data.pressure < 1000)
          break;
        nanosleep((const struct timespec[]){{0, 250000000L}}, NULL);

        ++retries;
        if (retries > 10) {
          syslog(LOG_ERR, "Could not obtain realistic data from sensor number %d\n", i);
          return SENSOR_FAIL;
        }
      }

      reply = redisCommand(c, "HGET %s open", bme_sensors[i].name);
      syslog(LOG_NOTICE, "Sensor %d had open state %s", i, reply->str);

      if (!strcmp(reply->str, "1")) {
        syslog(LOG_NOTICE, "Sensor %d had open avg. %s", i, reply->str);
        freeReplyObject(reply);
        reply = redisCommand(c, "HGET %s openavg", bme_sensors[i].name);

        if (reply->str && atof(reply->str)) {
          bme_sensors[i].open_average = atof(reply->str) + pressure_delta;
          bme_sensors[i].strikes_closed = WINDOW_SIZE;
          bme_sensors[i].average = bme_sensors[i].open_average - 0.3;
          for (int j = 0; j < WINDOW_SIZE; j++)
            bme_sensors[i].window[j] = bme_sensors[i].open_average;
        }
      } else {
        for (int j = 0; j < WINDOW_SIZE; j++)
          bme_sensors[i].window[j] = avg;
      }
    }
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "RPUSH valid_sensors %s", bme_sensors[i].name);
    freeReplyObject(reply);
    update_open(&bme_sensors[i]);
  }

  syslog(LOG_NOTICE, "Calibration data obtained");
  int i = 0;

  reply = (redisReply*)redisCommand(c, "SET retries 0");
  freeReplyObject(reply);

  uint8_t bme_errors = 0;

  while (1) {
    for (i = 0; i < valid_bme; i++) {
      if (bme_read(&bme_sensors[i].dev, &bme_sensors[i].data) == BME280_OK &&
          check_alteration(bme_sensors[i]) == BME280_OK) {
        bme_errors = 0;
        reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", bme_sensors[i].name, "temperature",
                                          bme_sensors[i].data.temperature);
        if (reply == NULL)
          return DB_FAIL;

        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", bme_sensors[i].name, "pressure",
                                          bme_sensors[i].data.pressure);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", bme_sensors[i].name, "humidity",
                                          bme_sensors[i].data.humidity);
        freeReplyObject(reply);

        update_open(&bme_sensors[i]);
        reply = (redisReply*)redisCommand(c, "HSET %s %s %d", bme_sensors[i].name, "open",
                                          bme_sensors[i].is_open);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", bme_sensors[i].name, "avg",
                                          bme_sensors[i].average);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", bme_sensors[i].name, "openavg",
                                          bme_sensors[i].open_average);
        freeReplyObject(reply);

        bme_sensors->past_pres = bme_sensors[i].data.pressure;
      } else {
        if (bme_errors++ > ERROR_THRESHOLD)
          return SENSOR_FAIL;
      }
    }

    for (i = 0; i < valid_sht; i++) {
      if (sht3x_measure_blocking_read(&sht_sensors[i]) != BME280_OK)
        return SENSOR_FAIL;

      reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", sht_sensors[i].name, "temperature",
                                        sht_sensors[i].data.temperature);

      if (reply == NULL)
        return DB_FAIL;

      freeReplyObject(reply);

      reply = (redisReply*)redisCommand(c, "HSET %s %s %.3f", sht_sensors[i].name, "humidity",
                                        sht_sensors[i].data.humidity);
      freeReplyObject(reply);
    }

    if (iface_board_len == 3)
      unselect_i2c_extender();

    reply_remote = (redisReply*)redisCommand(c_remote, "GET wgen2_pressure");

    if (reply_remote->str) {
      reply = (redisReply*)redisCommand(c, "SET last_ext_pressure %s", reply_remote->str);
      freeReplyObject(reply);
    }

    freeReplyObject(reply_remote);
    nanosleep((const struct timespec[]){{0, 250000000L}}, NULL);
  }

  redisFree(c);
  return 0;
}
