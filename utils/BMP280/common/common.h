/**\
 * Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/******************************************************************************/
#ifndef BMP_COMMON_H
#define BMP_COMMON_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

#include "../../I2C/common.h"
#include "../bmp2.h"

struct sensor_data {
  /*! Last valid pressure reading */
  double past_pres;
  /*! Closed door pressure average */
  double average;
  /*! Open door pressure average */
  double open_average;
  /*! Device compensated data */
  struct bmp2_data data;
  /*! Moving average window */
  double window[WINDOW_SIZE];
  /*! BMP280 device */
  struct bmp2_dev dev;
  /*! Number of times sensor was significantly above the average closed door
   * pressure */
  uint8_t strikes_closed;
  /*! Rack door status */
  uint8_t is_open;
  /*! Identifier*/
  struct identifier id;
  /*! Device config */
  struct bmp2_config config;
  /*! Sensor nickname */
  char name[MAX_NAME_LEN];
};

/***************************************************************************/

/*!                 User function prototypes
 ****************************************************************************/

/*!
 *  @brief Function for reading the sensor's registers through SPI bus.
 *
 *  @param[in] reg_addr   : Register address.
 *  @param[out] reg_data  : Pointer to the data buffer to store the read data.
 *  @param[in] length     : No of bytes to read.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return Status of execution
 *
 *  @retval BMP2_INTF_RET_SUCCESS -> Success.
 *  @retval != BMP2_INTF_RET_SUCCESS -> Failure.
 *
 */
BMP2_INTF_RET_TYPE bmp2_spi_read(uint8_t reg_addr,
                                 uint8_t* reg_data,
                                 uint32_t length,
                                 void* intf_ptr);

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr   : Register address.
 *  @param[out] reg_data  : Pointer to the data buffer to store the read data.
 *  @param[in] length     : No of bytes to read.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return Status of execution
 *
 *  @retval BMP2_INTF_RET_SUCCESS -> Success.
 *  @retval != BMP2_INTF_RET_SUCCESS -> Failure.
 *
 */
BMP2_INTF_RET_TYPE bmp2_i2c_read(uint8_t reg_addr,
                                 uint8_t* reg_data,
                                 uint32_t length,
                                 void* intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through SPI bus.
 *
 *  @param[in] reg_addr   : Register address.
 *  @param[in] reg_data   : Pointer to the data buffer whose data has to be
 * written.
 *  @param[in] length     : No of bytes to write.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return Status of execution
 *
 *  @retval BMP2_INTF_RET_SUCCESS -> Success.
 *  @retval != BMP2_INTF_RET_SUCCESS -> Failure.
 *
 */
BMP2_INTF_RET_TYPE bmp2_spi_write(uint8_t reg_addr,
                                  const uint8_t* reg_data,
                                  uint32_t length,
                                  void* intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr : Register address.
 *  @param[in] reg_data : Pointer to the data buffer whose value is to be
 * written.
 *  @param[in] length   : No of bytes to write.
 *  @param[in] intf_ptr : Interface pointer
 *
 *  @return Status of execution
 *
 *  @retval BMP2_INTF_RET_SUCCESS -> Success.
 *  @retval != BMP2_INTF_RET_SUCCESS -> Failure.
 *
 */
BMP2_INTF_RET_TYPE bmp2_i2c_write(uint8_t reg_addr,
                                  const uint8_t* reg_data,
                                  uint32_t length,
                                  void* intf_ptr);

/*!
 *  @brief This function provides the delay for required time (Microsecond) as
 * per the input provided in some of the APIs.
 *
 *  @param[in] period_us  : The required wait time in microsecond.
 *  @param[in] intf_ptr   : Interface pointer
 *
 *  @return void.
 */
void bmp2_delay_us(uint32_t period_us, void* intf_ptr);

/*!
 *  @brief This function is to select the interface between SPI and I2C.
 *
 *  @param[in] dev    : Structure instance of bmp2_dev
 *  @param[in] intf   : Interface selection parameter
 *                          For I2C : BMP2_I2C_INTF
 *                          For SPI : BMP2_SPI_INTF
 *
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval < 0 -> Failure
 */
int8_t bmp2_interface_selection(struct sensor_data* sensor, uint8_t intf, uint8_t addr);

/*!
 *  @brief This API is used to print the execution status.
 *
 *  @param[in] api_name : Name of the API whose execution status has to be
 * printed.
 *  @param[in] rslt     : Error code returned by the API whose execution status
 * has to be printed.
 *
 *  @return void.
 */
void bmp2_error_codes_print_result(const char api_name[], int8_t rslt);

/*!
 * @brief This function deinitializes coines platform
 *
 *  @return void.
 *
 */
void bmp2_coines_deinit(void);

/*!
 * @brief Initializes and configures sensor.
 *
 *  @param[in] sensor : BMP sensor to initialize
 *  @param[in] addr   : Address for the sensor (x76 or x77)
 *  @return Status of execution.
 *
 */
int8_t bmp_init(struct sensor_data* sensor, uint8_t addr);

/*!
 * @brief Reads temperature/pressure data
 *
 *  @param[in] sensor : BMP sensor to read data from
 *  @return Status of execution.
 *
 */
int8_t bmp_read(struct sensor_data* sensor);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* _COMMON_H */
