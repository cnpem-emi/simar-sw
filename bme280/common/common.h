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
int8_t set_ext_addr(uint8_t addr);

/*!
 * @brief Parent struct for all valid sensors, including custom values to aid in door status calibration.
 */
struct sensor_data {
  double past_pres; /**< Last pressure measurement (used for checking for abnormalities) */
  double average; /**< Pressure moving average for closed doors */
  double open_average; /**< Pressure moving average for open doors */
  struct bme280_data data; /**< BME280 data struct */
  double window[WINDOW_SIZE]; /**< Moving average window */
  struct bme280_dev dev; /**< BME280 device struct */
  uint8_t strikes_closed; /**< Door status threshold under/over count */
  uint8_t is_open; /**< Door status */
  struct identifier id; /**< Sensor identification struct */
  char name[MAX_NAME_LEN]; /**< Sensor name */
};

int8_t check_alteration(struct sensor_data sensor);

#endif
