#ifndef BME_COMMON_H
#define BME_COMMON_H

#include "../bme2.h"
#include "../../I2C/common.h"

#define WINDOW_SIZE 5
#define MAX_NAME_LEN 16
#define SENSOR_FAIL -2
#define DB_FAIL -3

int8_t bme_read(struct bme280_dev *dev, struct bme280_data *comp_data);
int8_t bme_init(struct bme280_dev *dev, struct identifier *id, uint8_t address);

struct sensor_data {
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

#endif
