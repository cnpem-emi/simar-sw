#include <pthread.h>
#include <hiredis/hiredis.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>

#include "SPI/common.h"

#define OUTLET_QUANTITY 8
#define RESOLUTION 0.01953125
#define PRU_DEVICE_NAME "/dev/rpmsg_pru30"

const char servers[11][16] = { 
    "10.0.38.59",
    "10.0.38.46",
    "10.0.38.42",
    "10.128.153.81",
    "10.128.153.82",
    "10.128.153.83",
    "10.128.153.84",
    "10.128.153.85",
    "10.128.153.86",
    "10.128.153.87",
    "10.128.153.88",
};

redisContext *c, *c_remote;
char name[72];
pthread_mutex_t spi_mutex;

void connect_remote() {
    int server_i = 0;
    int server_amount = sizeof(servers)/sizeof(servers[0]);

    do {
        c_remote = redisConnectWithTimeout(
            servers[server_i], 6379, (struct timeval) { 1, 500000 });

        if (c_remote->err) {
            syslog(LOG_ERR,
                "%s remote Redis server not available, switching...\n",
                servers[server_i++]);

            nanosleep((const struct timespec[]) { { 1, 0 } }, NULL); // 1s
        }

        if(server_i == server_amount) {
            syslog(LOG_ERR, "No server found");
            exit(-3);
        }
    } while (c_remote->err);
}

void *command_listener() {
    redisReply *reply, *up_reply;

    connect_remote();

    syslog(LOG_NOTICE, "Redis command DB connected");
    uint8_t command, outlet;

    const struct timespec *period = (const struct timespec[]){ { 1, 0 } };
    while (1) {
        reply = redisCommand(c, "HMGET device ip_address name");

        if(reply->type == REDIS_REPLY_ARRAY && reply->elements == 2)
            snprintf(name, 64, "SIMAR:%s:%s", reply->element[0]->str, reply->element[1]->str);

        freeReplyObject(reply);

        reply = redisCommand(c_remote,"LPOP %s", name);

        if(reply->type == REDIS_REPLY_STRING && strlen(reply->str) > 0) {
            command = reply->str[0] - '0';
            outlet = reply->str[2] - '0';

            if(outlet > 8) {
                syslog(LOG_ERR, "Received malformed command: %s", reply->str);
                freeReplyObject(reply);
                continue;
            }

            pthread_mutex_lock (&spi_mutex); // Lock SPI to prevent voltage/current readout thread from messing with outlets
            select_module(0,3);

            switch(command) {
                case 0:
                    write_data(0b00000, "\x00", 1);
                    syslog(LOG_NOTICE, "User %s switched outlet %d off", reply->str+4, outlet);
                    // Write new outlet status to server, so that the GUI can use it
                    up_reply = redisCommand(c_remote, "SREM %s:Outlets %d", name, outlet);
                    freeReplyObject(up_reply);
                break;
                case 1:
                    write_data(0b00000, "\xff", 1);
                    syslog(LOG_NOTICE, "User %s switched outlet %d on", reply->str+4, outlet);
                    up_reply = redisCommand(c_remote, "SADD %s:Outlets %d", name, outlet);
                    freeReplyObject(up_reply);
                break;
            }

            select_module(0, 24);
            pthread_mutex_unlock (&spi_mutex);

        } else if (reply->type == REDIS_REPLY_ERROR) {
            connect_remote();
        }

        freeReplyObject(reply);
        nanosleep(period, NULL);
    }
}

void *glitch_counter() { 
    struct pollfd prufd;

    prufd.fd = open(PRU_DEVICE_NAME, O_RDWR);
    char buf[512];
    redisReply *reply;

    if (prufd.fd < 0)
    {
        syslog(LOG_ERR, "Failed to communicate with PRU");
        exit(-9);
    }

    for(;;) {
        write(prufd.fd, "-", 1);
        usleep(999600); // There is a 400 us offset, we're aiming for 1s
        write(prufd.fd, "-", 1);

        if (read(prufd.fd, buf, 512))
        {
            reply = redisCommand(c, "SET glitch_count %d", (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
            freeReplyObject(reply);
            read(prufd.fd, buf, 512);
        }
    }
}
double calc_voltage(char *buffer) {
    return (((buffer[1] & 0x0F)*16) + ((buffer[0] & 0xF0) >> 4))*RESOLUTION;
}

int main(int argc, char* argv[])
{
    openlog("simar_volt", 0, LOG_LOCAL0);
    redisReply *reply;

    syslog(LOG_NOTICE, "Starting up...");

    do {
        c = redisConnectWithTimeout("127.0.0.1", 6379, (struct timeval) { 1, 500000 });

        if (c->err) {
            if (c->err == 1)
                syslog(LOG_ERR,
                    "Redis server instance not available. Have you "
                    "initialized the Redis server? (Error code 1)\n");
            else
                syslog(LOG_ERR, "Unknown redis error (error code %d)\n", c->err);

            nanosleep((const struct timespec[]) { { 0, 700000000L } }, NULL); // 700ms
        }

    } while (c->err);

    syslog(LOG_NOTICE, "Redis voltage DB connected");

    char buffer[3];
    double current = 0, voltage = 0, peak_voltage = 0;
    uint32_t mode = 1;
    uint8_t bpw = 16;
    uint32_t speed = 2000000;

    pthread_mutex_init(&spi_mutex, NULL);

    spi_open("/dev/spidev0.0", &mode, &bpw, &speed);

    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, command_listener, NULL);

    pthread_t glitch_thread;
    pthread_create(&glitch_thread, NULL, glitch_counter, NULL);

    select_module(0, 24);

    // Dummy conversions
    spi_transfer("\x0F\x0F", buffer, 2);
    spi_transfer("\x0F\x0F", buffer, 2);

    spi_transfer("\x10\x83", buffer, 2);

    int i = 0;

    const struct timespec *period = (const struct timespec[]){{ 0, 800000000L }};
    while(1) {
        pthread_mutex_lock (&spi_mutex);
        peak_voltage = 0;

        // Current
        spi_transfer("\x10\x83", buffer, 2);
        spi_transfer("\x10\x83", buffer, 2);
        current = calc_voltage(buffer);
        current = (current - 2.5)/0.125;

        // Throwaway
        spi_transfer("\x10\x9f", buffer, 2);

        for(i = 0; i < 240; i++) {
            spi_transfer("\x10\x9f", buffer, 2);
            voltage = calc_voltage(buffer);
            peak_voltage = voltage > peak_voltage ? voltage : peak_voltage;
        } // Takes around 50 ms

        pthread_mutex_unlock(&spi_mutex);

        reply = redisCommand(c, "SET vch_%d %.3f", 0, peak_voltage*31.5);
        freeReplyObject(reply);

        reply = redisCommand(c, "SET ich_%d %.3f", 0, current);
        freeReplyObject(reply);

        nanosleep(period, NULL);
    }
}
