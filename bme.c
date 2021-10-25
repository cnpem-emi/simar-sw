#include <hiredis/hiredis.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include "BME280/common/common.h"

const char servers[1][11] = {"10.15.0.254"};

/**
 * @brief Updates door opening status
 *
 * @param[in] sensor : Pointer to sensor
 *
 * @return void
 */
void get_open_iter(struct sensor_data* sensor) {
  /* Considering the closed server racks work under negative pressure, a
   sustained "significant" rise in pressure may indicate a door opening.

   However, a change in temperature may (albeit slowly) alter the pressure,
   so the best solution is to maintain a moving open/closed pressure average
   and listen for sudden changes.
  */

  // 0.23 refers to the standard deviation of the population over a period of 3
  // minutes
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
 * @return void
 */
void update_open(struct sensor_data* sensor) {
  // Moves moving average window 1 position to the left to accomodate new value
  double cache[WINDOW_SIZE];
  for (int i = 0; i < WINDOW_SIZE; i++)
    cache[i] = sensor->window[i];

  for (int i = WINDOW_SIZE - 1; i > 0; i--)
    sensor->window[i - 1] = cache[i];

  sensor->window[WINDOW_SIZE - 1] = sensor->data.pressure;

  /* The "average" pressure is important to determine sudden changes, however,
  it'll only respond to gradual changes in order to deter statistical
  abnormalities.

  As for the "open" average, it'll get reset every time the door is closed,
  which is the default state.

  The closed door pressure moving average ignores sudden large pressure
  increases, but is more sensitive to pressure decreases. The opposite happens
  to the open door pressure moving average. */

  double sum = 0;

  for (int i = 0; i < WINDOW_SIZE; i++)
    sum += sensor->window[i];

  double diff = sensor->average - sensor->data.pressure;
  double open_diff = sensor->open_average - sensor->data.pressure;

  if ((sensor->is_open && sensor->strikes_closed == WINDOW_SIZE) ||
      (!sensor->is_open && sensor->strikes_closed == 0)) {
    if (sensor->average == 0 || (diff > -0.08 && diff < 0.1 && !sensor->is_open))
      sensor->average = sum / WINDOW_SIZE;
    else if (sensor->is_open &&
             (sensor->open_average == 0 || (open_diff > -0.1 && open_diff < 0.08)))
      sensor->open_average = sum / WINDOW_SIZE;
  }

  get_open_iter(sensor);
}

/**
 * @brief Checks if the alteration in pressure over one measurement is realistic
 *
 * @param[in] sensor : Sensor to check
 *
 * @return void
 */
int8_t check_alteration(struct sensor_data sensor) {
  if (sensor.data.pressure == 1100 || sensor.data.pressure < 1) {
    syslog(LOG_CRIT,
           "Sensor %s failed to respond with valid measurements, unlikely to "
           "recover, reinitializing all sensors",
           sensor.name);
    exit(SENSOR_FAIL);
  }

  return sensor.past_pres == 0 ||
         (fabs(sensor.past_pres - sensor.data.pressure) < 2 && sensor.data.pressure > 800 &&
          sensor.data.pressure < 1000 && sensor.data.humidity != 100);
}

int main(int argc, char* argv[]) {
  openlog("simar_bme", 0, LOG_LOCAL0);

  if (!--argc)
    syslog(LOG_INFO, "You should consider using at least one custom name");

  redisContext *c, *c_remote;
  redisReply *reply, *reply_remote;

  struct sensor_data sensors[8];

  sensors[0].dev.settings.osr_h = BME280_OVERSAMPLING_4X;
  sensors[0].dev.settings.osr_p = BME280_OVERSAMPLING_16X;
  sensors[0].dev.settings.osr_t = BME280_OVERSAMPLING_4X;
  sensors[0].dev.settings.filter = BME280_FILTER_COEFF_OFF;

  uint8_t valid_i = 0;

  for (int i = 0; i < 8; i++) {
    struct sensor_data sensor;
    sensor.dev = sensors[0].dev;
    sensor.id.mux_id = i % 4;

    if (bme_init(&sensor.dev, &sensor.id, i < 4 ? BME280_I2C_ADDR_PRIM : BME280_I2C_ADDR_SEC) ==
        BME280_OK) {
      sensors[valid_i] = sensor;
      sensors[valid_i].dev.intf_ptr = &sensors[valid_i].id;
      sensors[valid_i].average = sensors[valid_i].open_average = 0;
      sensors[valid_i].strikes_closed = sensors[valid_i].is_open = 0;

      if (valid_i < argc)
        snprintf(sensors[valid_i].name, MAX_NAME_LEN, "%s", argv[valid_i + 1]);
      else
        snprintf(sensors[valid_i].name, MAX_NAME_LEN, "sensor_%d_%x", i % 4,
                 i < 4 ? BME280_I2C_ADDR_PRIM : BME280_I2C_ADDR_SEC);

      valid_i++;
    }
  }

  if (!valid_i || (argc && valid_i < argc)) {
    syslog(LOG_CRIT, "Not enough sensors found");
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

  reply_remote = redisCommand(c_remote, "GET pressure_B15");

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

  for (int i = 0; i < valid_i; i++) {
    reply = redisCommand(c, "GET avg_%s", sensors[i].name);

    if (!reply->str) {
      sensors[i].past_pres = 0;
      for (int j = 0; j < WINDOW_SIZE; j++) {
        bme_read(&sensors[i].dev, &sensors[i].data);

        if (sensors[i].data.pressure > 800 && sensors[i].data.pressure < 1000) {
          sensors[i].window[j] = sensors[i].past_pres = sensors[i].data.pressure;
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
      sensors[i].past_pres = avg;

      while (1) {
        bme_read(&sensors[i].dev, &sensors[i].data);
        if (sensors[i].data.pressure > 900 && sensors[i].data.pressure < 1000)
          break;
      }

      ++retries;
      if (retries > 10) {
        syslog(LOG_ERR, "Could not obtain realistic data from sensor number %d\n", i);
        return SENSOR_FAIL;
      }

      reply = redisCommand(c, "GET open_%s", sensors[i].name);
      syslog(LOG_NOTICE, "Sensor %d had open state %s", i, reply->str);

      if (!strcmp(reply->str, "1")) {
        syslog(LOG_NOTICE, "Sensor %d had open avg. %s", i, reply->str);
        freeReplyObject(reply);
        reply = redisCommand(c, "GET openavg_%s", sensors[i].name);

        if (reply->str && atof(reply->str)) {
          sensors[i].open_average = atof(reply->str) + pressure_delta;
          sensors[i].strikes_closed = WINDOW_SIZE;
          sensors[i].average = sensors[i].open_average - 0.3;
          for (int j = 0; j < WINDOW_SIZE; j++)
            sensors[i].window[j] = sensors[i].open_average;
        }
      } else {
        for (int j = 0; j < WINDOW_SIZE; j++)
          sensors[i].window[j] = avg;
      }
    }
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "RPUSH valid_sensors %s", sensors[i].name);
    freeReplyObject(reply);
    update_open(&sensors[i]);
  }

  syslog(LOG_NOTICE, "Calibration data obtained");
  int i = 0;

  reply = (redisReply*)redisCommand(c, "SET retries 0");
  freeReplyObject(reply);

  while (1) {
    for (i = 0; i < valid_i; i++) {
      bme_read(&sensors[i].dev, &sensors[i].data);
      if (check_alteration(sensors[i])) {
        reply = (redisReply*)redisCommand(c, "SET %s_%s %.3f", "temperature", sensors[i].name,
                                          sensors[i].data.temperature);
        if (reply->integer)
          return DB_FAIL;
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "SET %s_%s %.3f", "pressure", sensors[i].name,
                                          sensors[i].data.pressure);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "SET %s_%s %.3f", "humidity", sensors[i].name,
                                          sensors[i].data.humidity);
        freeReplyObject(reply);

        update_open(&sensors[i]);
        reply = (redisReply*)redisCommand(c, "SET %s_%s %d", "open", sensors[i].name,
                                          sensors[i].is_open);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "SET %s_%s %.3f", "avg", sensors[i].name,
                                          sensors[i].average);
        freeReplyObject(reply);

        reply = (redisReply*)redisCommand(c, "SET %s_%s %.3f", "openavg", sensors[i].name,
                                          sensors[i].open_average);
        freeReplyObject(reply);

        sensors->past_pres = sensors[i].data.pressure;
      }
      nanosleep((const struct timespec[]){{0, 250000000L}}, NULL);
    }

    reply_remote = (redisReply*)redisCommand(c_remote, "GET B15_pressure");

    if (reply_remote->str) {
      reply = (redisReply*)redisCommand(c, "SET last_ext_pressure %s", reply_remote->str);
      freeReplyObject(reply);
    }

    freeReplyObject(reply_remote);
  }

  redisFree(c);
  return 0;
}
