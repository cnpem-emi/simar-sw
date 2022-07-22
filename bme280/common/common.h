/*! @file common.h
 * @brief Common declarations for BME280 operation on AM335x
 */

#ifndef BME_COMMON_H
#define BME_COMMON_H

#include "../../i2c/common.h"
#include "../bme2.h"

#define WINDOW_SIZE 5
#define MAX_NAME_LEN 16

int8_t bme_read(struct bme280_dev* dev, struct bme280_data* comp_data);
int8_t bme_init(struct bme280_dev* dev, struct identifier* id, uint8_t address);

/*!
 * @brief Parent struct for all valid BMx sensors, including custom values to aid in door status
 * calibration.
 */
struct bme_sensor_data {
  double past_pres;
  double average;
  double open_average;
  struct bme280_data data;
  double window[WINDOW_SIZE];
  struct bme280_dev dev;
  uint8_t strikes_closed;
  uint8_t is_open;
  struct identifier id;
  char name[MAX_NAME_LEN];
};

/**
 * @brief Checks if the alteration in pressure over one measurement is realistic
 *
 * @param[in] sensor : Sensor to check
 *
 * @return Sensor data alteration status
 * @retval 0 Sensor is not malfunctioning (outputting valid data)
 * @retval -1 Sensor is possibily malfunctioning
 */
int8_t check_alteration(struct bme_sensor_data sensor);

#endif
