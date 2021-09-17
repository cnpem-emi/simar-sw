#include <hiredis/hiredis.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>

#include "common/common.h"

double average = 0, open_average = 0;
double window[10];
int8_t strikes_closed = 0;
int8_t is_open = 0;

/**
 * @brief Checks if the alteration in pressure over one measurement is realistic
 *
 * @param[in] sensor : Sensor to check
 *
 * @return void
 */
int8_t check_alteration(struct sensor_data sensor)
{
    if (sensor.data.pressure == 1100) {
        syslog(LOG_CRIT,
            "Sensor %s failed to respond with valid measurements, unlikely to "
            "recover, reinitializing all sensors",
            sensor.name);
        exit(-3);
    }

    return sensor.past_pres == 0 || (fabs(sensor.past_pres - sensor.data.pressure) < sensor.past_pres / 7 && sensor.data.pressure > 800 && sensor.data.pressure < 1000 && sensor.data.humidity != 100);
}

int main(int argc, char *argv[])
{
    redisContext *c;

    struct bme280_dev int_bme;
    struct identifier int_id;

    struct timeval redis_timeout = {1, 500000};

    struct sensor_data sensor;

    int_bme.settings.osr_h = BME280_OVERSAMPLING_4X;
    int_bme.settings.osr_p = BME280_OVERSAMPLING_16X;
    int_bme.settings.osr_t = BME280_OVERSAMPLING_4X;
    int_bme.settings.filter = BME280_FILTER_COEFF_OFF;

    int_id.mux_id = 0;

    bme_init(&int_bme, &int_id, BME280_I2C_ADDR_PRIM);
    strcpy(sensor.name, "");
    sensor.past_pres = 0;

    do
    {
        c = redisConnectWithTimeout("127.0.0.1", 6379, redis_timeout);
        if (c->err)
        {
            if (c->err == 1)
                syslog(LOG_ERR,"Redis server instance not available. Have you initialized the Redis server? (Error code 1)");
            else
                syslog(LOG_ERR,"Unknown redis error (error code %d)\n", c->err);

            sensor.dev.delay_us(700000, sensor.dev.intf_ptr);
        }
    } while (c->err);

    syslog(LOG_INFO, "Connected to Redis DB");

    redisReply *reply;

    openlog("simar_bme", 0, LOG_LOCAL0);
    if (strcmp(getenv("WIRELESS"), "true") != 0)
        setlogmask(LOG_WARNING);

    while (1)
    {
        bme_read(&int_bme, &sensor.data);

        if (check_alteration(sensor))
        {
            syslog(LOG_DEBUG, "%ld,%.3f,%.3f,%.3f", (unsigned long)time(NULL), sensor.data.temperature, sensor.data.pressure, sensor.data.humidity);
            reply = (redisReply *)redisCommand(c, "SET %s%s %.3f", "temperature", sensor.name, sensor.data.temperature);
            freeReplyObject(reply);

            reply = (redisReply *)redisCommand(c, "SET %s%s %.3f", "pressure", sensor.name, sensor.data.pressure);
            freeReplyObject(reply);

            reply = (redisReply *)redisCommand(c, "SET %s%s %.3f", "humidity", sensor.name, sensor.data.humidity);
            freeReplyObject(reply);

            sensor.past_pres = sensor.data.pressure;
        }

        sensor.dev.delay_us(950000, sensor.dev.intf_ptr);
    }

    redisFree(c);
    return 0;
}
