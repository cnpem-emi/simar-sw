/*! @file common.h
 * @brief Common declarations for I2C operations
 */

/*!
 * @defgroup i2c I2C
 * @brief I2C communication
 */

#ifndef I2C_COMMON_H
#define I2C_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#define WINDOW_SIZE 5
#define MAX_NAME_LEN 16

#include "../spi/common.h"

/*!
 * @brief Identification/communication structure for I2C devices
 */
struct identifier {
  int8_t ext_mux_id;
  uint8_t mux_id;
  uint8_t fd;
};

/**
 * \ingroup i2c
 * \defgroup i2cComm Communication
 * @brief Communication methods
 */

/*!
 * \ingroup i2cComm
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[out] data          : Pointer to the data buffer to store the read
 * data.
 *  @param[in] len            : No of bytes to read.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs.
 *
 *  @return Execution status
 *
 *  @retval 0 -> Success
 *  @retval -1 -> Failure
 */
int8_t i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr);

/*!
 *  \ingroup i2cComm
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[in] data           : Pointer to the data buffer whose value is to be
 * written.
 *  @param[in] len            : No of bytes to write.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs
 *
 *  @return Execution status
 *
 *  @retval BME280_OK -> Success
 *  @retval BME280_E_COMM_FAIL -> Communication failure.
 *
 */
int8_t i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr);

/*!
 *  \ingroup i2cComm
 *  @brief Opens communication with the I2C bus at a given address
 *
 *  @param[out] fd            : Pointer for the file descriptor
 *  @param[in] addr           : Address of the connected I2C device
 *
 *  @return Execution status
 *  @retval BME280_OK -> Success
 *  @retval BME280_E_COMM_FAIL -> Communication failure.
 */
int8_t i2c_open(int8_t* fd, uint8_t addr);

/**
 * \ingroup i2c
 * \defgroup i2cMux Multiplexer and Extension Boards
 * @brief Generic methods for handling communication with the multiplexer and extension boards
 */

/**
 * \ingroup i2cMux
 * @brief Sets the SPI extender board address
 * @param[in] addr Board address
 * @retval 0 OK
 * @retval -1 Invalid board address
 */
int8_t set_ext_addr(uint8_t addr);

/**
 * \ingroup i2cMux
 * @brief Unselects the I2C extender (and SPI extender, by proxy)
 * @return void
 */
void unselect_i2c_extender();

/**
 * \ingroup i2cMux
 * @brief Selects an available I2C channel through the SPI and I2C extender boards (0 to 8)
 * @param[in] id Desired channel ID
 * @param[in] addr Designed extender board address
 * @return void
 */
void direct_ext_mux(uint8_t id);

/**
 * \ingroup i2cMux
 * @brief Selects an available I2C channel through the digital interface board (0 to 4)
 * @param[in] id Desired channel ID
 * @return void
 */
void direct_mux(uint8_t id);

/**
 * \ingroup i2cMux
 * @brief Configures pins for the digital interface board multiplexing function
 * @return void
 */
int8_t configure_mux();

/**
 * @brief Delays execution for n us
 * @param[in] period Microsseconds to stop for
 * @param[in, out] intf_ptr Interface pointer, for BME driver compatibility (pass NULL)
 */
void delay_us(uint32_t period, void* intf_ptr);

#ifdef __cplusplus
}
#endif

#endif
