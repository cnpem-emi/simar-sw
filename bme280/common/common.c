#include <stdlib.h>
#include <syslog.h>

#include "common.h"

int8_t fd_76 = 0;
int8_t fd_77 = 0;

/**
 * @brief Initializes sensor communication
 * @param[in] dev BME280/BMP280 device
 * @param[in] id Sensor identification struct (channel)
 * @param[in] addr Sensor address (0x76 or 0x77) 
 * @retval 0 OK
 * @retval -2 Communication failure
 */
int8_t bme_init(struct bme280_dev* dev, struct identifier* id, uint8_t addr) {
  int8_t rslt = BME280_OK;
  uint8_t settings_sel = 0;

  if (configure_mux()) {
    syslog(LOG_CRIT, "Failed to configure demux switching.\n");
    exit(1);
  }

  int8_t* fd = addr == 0x76 ? &fd_76 : &fd_77;

  if (i2c_open(fd, addr)) {
    syslog(LOG_CRIT, "Failed to open bus");
    exit(-2);
  }

  id->fd = *fd;
  dev->intf = BME280_I2C_INTF;
  dev->read = i2c_read;
  dev->write = i2c_write;
  dev->delay_us = delay_us;
  dev->intf_ptr = id;

  direct_mux(id->mux_id);

  if(id->ext_mux_id >= 0) direct_ext_mux(id->ext_mux_id, 15);

  rslt = bme280_init(dev);
  if (rslt != BME280_OK) {
    syslog(LOG_ALERT, "Failed to initialize the device %d at %x (code %+d).\n", id->mux_id, addr,
           rslt);
    return 2;
  }

  settings_sel =
      BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

  rslt = bme280_set_sensor_settings(settings_sel, dev);

  rslt = bme280_set_sensor_mode(BME280_NORMAL_MODE, dev);
  if (rslt != BME280_OK)
    return rslt;

  return rslt;
}

/**
 * @brief Reads sensor data
 * @param[in] dev BME280/BMP280 device
 * @param[out] comp_data Pointer to compensated data struct
 * @retval 0 OK
 * @retval -2 Communication failure
 */
int8_t bme_read(struct bme280_dev* dev, struct bme280_data* comp_data) {
  int8_t rslt = BME280_OK;

  struct identifier id;
  id = *((struct identifier*)dev->intf_ptr);

  direct_mux(id.mux_id);

  if(id.ext_mux_id >= 0) direct_ext_mux(id.ext_mux_id, 15);

  rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
  comp_data->pressure *= 0.01;

  return rslt;
}
