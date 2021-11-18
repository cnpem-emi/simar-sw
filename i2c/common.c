#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

gpio_t mux0 = {.pin = P9_15};  // LSB
gpio_t mux1 = {.pin = P9_16};  // MSB

int8_t pins_configured = 0;

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[out] data          : Pointer to the data buffer to store the read
 * data.
 *  @param[in] len            : No of bytes to read.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs.
 *
 *  @return Status of execution
 *
 *  @retval 0 -> Success
 *  @retval > 0 -> Failure Info
 *
 */
int8_t i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr) {
  struct identifier id;
  id = *((struct identifier*)intf_ptr);

  write(id.fd, &reg_addr, 1);
  return read(id.fd, reg_data, length) < 0;
}

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[in] data           : Pointer to the data buffer whose value is to be
 * written.
 *  @param[in] len            : No of bytes to write.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of
 * descriptors for interface related call backs
 *
 *  @return Status of execution
 *
 *  @retval BME280_OK -> Success
 *  @retval BME280_E_COMM_FAIL -> Communication failure.
 *
 */
int8_t i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr) {
  uint8_t* buf;

  struct identifier id;
  id = *((struct identifier*)intf_ptr);

  buf = malloc(length + 1);
  buf[0] = reg_addr;
  memcpy(buf + 1, reg_data, length);
  if (write(id.fd, buf, length + 1) < (uint16_t)length)
    return -2;

  free(buf);

  return 0;
}

void direct_mux(uint8_t id) {
  if ((id >> 0) & 1)
    bbb_mmio_set_high(mux0);
  else
    bbb_mmio_set_low(mux0);

  if ((id >> 1) & 1)
    bbb_mmio_set_high(mux1);
  else
    bbb_mmio_set_low(mux1);
}

int8_t configure_mux() {
  int8_t rslt = 0;

  if (!pins_configured) {
    rslt |= bbb_mmio_get_gpio(&mux0);
    bbb_mmio_set_output(mux0);
    rslt |= bbb_mmio_get_gpio(&mux1);
    bbb_mmio_set_output(mux0);

    pins_configured = !rslt;
  }

  return rslt;
}

/**
 * @brief Delays execution for n us
 * @param[in] period Microsseconds to stop for
 */
void delay_us(uint32_t period, void* intf_ptr) {
  nanosleep((const struct timespec[]){{0, period * 1000}}, NULL);
}

int8_t i2c_open(int8_t* fd, uint8_t addr) {
  if (!*fd) {
    *fd = open("/dev/i2c-2", O_RDWR);
    if (*fd < 0)
      return -2;
    if (ioctl(*fd, I2C_SLAVE, addr) < 0)
      return -2;
  }
  return 0;
}
