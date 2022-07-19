/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * \brief Sensirion SHT driver interface
 *
 * This module provides access to the SHT functionality over a generic I2C
 * interface. It supports measurements without clock stretching only.
 */

#ifndef SHT3X_H
#define SHT3X_H

#include "../i2c/common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STATUS_OK 0
#define STATUS_ERR_BAD_DATA (-1)
#define STATUS_CRC_FAIL (-2)
#define STATUS_UNKNOWN_DEVICE (-3)
#define STATUS_ERR_INVALID_PARAMS (-4)
#define SHT3X_MEASUREMENT_DURATION_USEC 15000

/* status word macros */
#define SHT3X_IS_ALRT_PENDING(status) (((status)&0x8000U) != 0U)
#define SHT3X_IS_ALRT_RH_TRACK(status) (((status)&0x0800) != 0U)
#define SHT3X_IS_ALRT_T_TRACK(status) (((status)&0x0400U) != 0U)
#define SHT3X_IS_SYSTEM_RST_DETECT(status) (((status)&0x0010U) != 0U)
#define SHT3X_IS_LAST_CRC_FAIL(status) (((status)&0x0001U) != 0U)

/*!
 * @brief Struct for all valid SHT3x readout data.
 */
struct sht3x_data {
  double temperature;
  double humidity;
};

/*!
 * @brief Parent struct for identification and readout data for the SHT3x.
 */
struct sht3x_sensor_data {
  double past_pres;
  struct sht3x_data data;
  struct identifier id;
  char name[MAX_NAME_LEN];
};

/**
 * @brief SHT3x I2C 7-bit address option
 */
typedef enum _sht3x_i2c_addr {
  SHT3X_I2C_ADDR_DFLT = 0x44,
  SHT3X_I2C_ADDR_ALT = 0x45
} sht3x_i2c_addr_t;

/**
 * @brief SHT3x measurment mode options (Low, Medium and High rerefresh rates)
 */
typedef enum _sht3x_measurement_mode {
  SHT3X_MEAS_MODE_LPM, /*low power mode*/
  SHT3X_MEAS_MODE_MPM, /*medium power mode*/
  SHT3X_MEAS_MODE_HPM  /*high power mode*/
} sht3x_measurement_mode_t;

/**
 * @brief SHT3x Alert Thresholds
 */
typedef enum _sht3x_alert_thd {
  SHT3X_HIALRT_SET,
  SHT3X_HIALRT_CLR,
  SHT3X_LOALRT_CLR,
  SHT3X_LOALRT_SET,
} sht3x_alert_thd_t;

/**
 * @brief Detects if a sensor is connected by reading out the ID register.
 * If the sensor does not answer or if the answer is not the expected value,
 * the test fails.
 *
 * @param[in] sensor                : Sensor Struct
 *
 * @return 0 if a sensor was detected
 */
int16_t sht3x_probe(struct sht3x_sensor_data* sensor);

/**
 * @brief Read the sensor status word
 *
 * @param[in] sensor                : Sensor struct
 * @param[out] status               : The address for the result of the status word
 *
 * @return 0 if the command was successful, else an error code
 */
int16_t sht3x_get_status(struct sht3x_sensor_data* sensor, uint16_t* status);

/**
 * @brief Clear the status register alert flags
 *
 * @param[in] sensor                : Sensor struct
 *
 * @return 0 if the command was successful, else an error code
 */
int16_t sht3x_clear_status(struct sht3x_sensor_data* sensor);

/**
 * @brief Starts a measurement and then reads out the results. This function
 * blocks while the measurement is in progress. The duration of the measurement
 * depends on the sensor in use, please consult the datasheet. Temperature is
 * returned in [degree Celsius], multiplied by 1000, and relative humidity in
 * [percent relative humidity], multiplied by 1000.
 *
 * @param[in, out] sensor           : Sensor struct
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_measure_blocking_read(struct sht3x_sensor_data* sensor);

/**
 * @brief Starts a measurement in high precision mode. Use sht3x_read() to read
 * out the values, once the measurement is done. The duration of the measurement
 * depends on the sensor in use, please consult the datasheet.
 *
 * @param[in] sensor                : Sensor Struct
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_measure(struct sht3x_sensor_data* sensor);

/**
 * @brief Reads out the results of a measurement that was previously started by
 * sht3x_measure(). If the measurement is still in progress, this function
 * returns an error.
 * Temperature is returned in [degree Celsius], multiplied by 1000,
 * and relative humidity in [percent relative humidity], multiplied by 1000.
 *
 * @param[in, out]  sensor          : Sensor struct
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_read(struct sht3x_sensor_data* sensor);

/**
 * @brief Enable or disable the SHT's low power mode
 *
 * @param[in] enable_low_power_mode : 1 to enable low power mode, 0 to disable
 */
void sht3x_enable_low_power_mode(uint8_t enable_low_power_mode);

/**
 * @brief Set the desired sensor's operating power mode
 *
 * @param[in] mode              : Power mode selector
 */
void sht3x_set_power_mode(sht3x_measurement_mode_t mode);

/**
 * @brief Read out the serial number
 *
 * @param[in]  sensor           : The sensor address
 * @param[out] serial           : The address for the result of the serial number
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_read_serial(struct sht3x_sensor_data* sensor, uint32_t* serial);

/**
 * @brief Set target temperature and humidity alert threshold
 *
 * @param[in] sensor            : The sensor address
 * @param[in] thd               : Target alert threshold to be edited
 * @param[in] humidity          : Humidity threshold in 1000*%RH
 * @param[in] temperature       : Temperature threshold in 1000*°C
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_set_alert_thd(struct sht3x_sensor_data* sensor,
                            sht3x_alert_thd_t thd,
                            uint32_t humidity,
                            int32_t temperature);

/**
 * @brief Get target temperature and humidity alert threshold
 *
 * @param[in]  addr             : The sensor address
 * @param[in]  thd              : Target alert threshold to be edited
 * @param[out] humidity         : Address for the result humidity thd in 1000*%RH
 * @param[out] temperature      : Address for the result temperature thd in 1000*°C
 *
 * @return 0 if the command was successful, else an error code.
 */
int16_t sht3x_get_alert_thd(struct sht3x_sensor_data* sensor,
                            sht3x_alert_thd_t thd,
                            int32_t* humidity,
                            int32_t* temperature);

/**
 * @brief converts to temperature from ADC ticks
 *
 * @param[in] tick              : Sensor ADC ticks
 * @param[out] temperature      : Temperature value in T°C*1000
 */
void tick_to_temperature(uint16_t tick, int32_t* temperature);

/**
 * @brief Converts to humidity from ADC ticks
 *
 * @param[in] tick              : Tick sensor ADC ticks
 * @param[out] humidity         : Humidity value in %*1000
 */
void tick_to_humidity(uint16_t tick, int32_t* humidity);

/**
 * @brief Converts temperature to ADC ticks
 *
 * @param[in] temperature        : Temperature value in T°C*1000
 * @param[out] tick              : Sensor ADC ticks
 */
void temperature_to_tick(int32_t temperature, uint16_t* tick);

/**
 * @brief Converts humidity to ADC ticks
 *
 * @param[in] humidity           : Humidity value in %*1000
 * @param[out] tick              : Sensor ADC ticks
 */
void humidity_to_tick(int32_t humidity, uint16_t* tick);

/**
 * @brief Initializes sensor communication
 *
 * @param[in] sht               : Sensor struct
 * @param[in] addr              : Sensor address (0x44 or 0x45)
 * @retval 0 OK
 * @retval -2 Communication failure
 */
int8_t sht3x_init(struct sht3x_sensor_data* sht, uint8_t addr);

#ifdef __cplusplus
}
#endif

#endif /* SHT3X_H */
