/**\
 * Copyright (c) 2021 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

#include <stdlib.h>
#include <syslog.h>

#include "common.h"

uint8_t fd_76 = 0;
uint8_t fd_77 = 0;

/*!
 *  @brief Function to select the interface between SPI and I2C.
 */
int8_t bmp2_interface_selection(struct sensor_data *sensor, uint8_t intf, uint8_t addr)
{
    int8_t rslt = BMP2_OK;

    if (sensor != NULL)
    {
        /* Bus configuration : I2C */
        if (intf == BMP2_I2C_INTF)
        {
            sensor->dev.read = i2c_read;
            sensor->dev.write = i2c_write;
            sensor->dev.intf = BMP2_I2C_INTF;

            uint8_t *fd = addr == 0x76 ? &fd_76 : &fd_77;

            if(i2c_open(fd, addr)) {
                syslog(LOG_CRIT, "Failed to open bus");
                exit(-2);
            }

            sensor->id.fd = *fd;
        }
        /* Bus configuration : SPI */
        else if (intf == BMP2_SPI_INTF)
        {
            /*printf("SPI Interface\n");

            dev_addr = COINES_SHUTTLE_PIN_7;
            dev->read = bmp2_spi_read;
            dev->write = bmp2_spi_write;
            dev->intf = BMP2_SPI_INTF;

            coines_config_spi_bus(COINES_SPI_BUS_0, COINES_SPI_SPEED_7_5_MHZ, COINES_SPI_MODE0);*/
            syslog(LOG_CRIT, "Not supported");
        }

        /* Holds the I2C device addr or SPI chip selection */
        sensor->dev.intf_ptr = &sensor->id;

        /* Configure delay in microseconds */
        sensor->dev.delay_us = delay_us;
    }
    else
    {
        rslt = BMP2_E_NULL_PTR;
    }

    return rslt;
}

/*!
 *  @brief Initializes and configures sensor.
 */
int8_t bmp_init(struct sensor_data *sensor, uint8_t addr) {
    int8_t rslt;

    if(configure_mux()) {
        syslog(LOG_CRIT, "Failed to configure demux switching.\n");
        exit(1);
    }

    direct_mux(sensor->id.mux_id);

    rslt = bmp2_interface_selection(sensor, BMP2_I2C_INTF, addr);
    if(rslt)
        return rslt;

    rslt = bmp2_get_config(&sensor->config, &sensor->dev);
    if(rslt)
        return rslt;

    sensor->config.os_mode = BMP2_OS_MODE_ULTRA_HIGH_RESOLUTION;
    sensor->config.filter = BMP2_FILTER_OFF;
    sensor->config.odr = BMP2_ODR_250_MS;

    rslt = bmp2_set_config(&sensor->config, &sensor->dev);
    if(rslt)
        return rslt;

    rslt = bmp2_set_power_mode(BMP2_POWERMODE_FORCED, &sensor->config, &sensor->dev);
    if(rslt)
        return rslt;

    return bmp2_init(&sensor->dev);
}

/*!
 *  @brief Reads temperature/pressure data.
 */
int8_t bmp_read(struct sensor_data *sensor) {
    struct bmp2_status status;
    int8_t rslt;

    direct_mux(sensor->id.mux_id);

    rslt = bmp2_get_status(&status, &sensor->dev);
    for(int i; i < 20;i++) {
        if (status.measuring == BMP2_MEAS_DONE) {
            sensor->dev.delay_us(1000, sensor->dev.intf_ptr);
            rslt = bmp2_get_sensor_data(&sensor->data, &sensor->dev);
            sensor->data.pressure /= 100;

            break;
        }
        bmp2_get_status(&status, &sensor->dev);
    }

    return rslt;
}
