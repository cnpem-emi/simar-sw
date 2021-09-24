#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <stdint.h>

#define DEVICE_NAME0 "/dev/rpmsg_pru30"
#define DEVICE_NAME1 "/dev/rpmsg_pru31"
#define MAX_BUFFER_SIZE 512

char readBuf[MAX_BUFFER_SIZE];

int8_t count_pru(uint32_t time_base, uint8_t pru, uint32_t *data) {
    struct pollfd pfd;
    uint8_t wr_stat;

    if(pru > 1) return -2;
    
    switch(pru) {
        case 0:
            pfd.fd = open(DEVICE_NAME0, O_RDWR);
            break;
        case 1:
            pfd.fd = open(DEVICE_NAME0, O_RDWR);
            break;
    }

    if (pfd.fd < 0) return -1;

    wr_stat = write(pfd.fd, "-", 2) == -1;
    usleep(time_base-400); // There is a 400 us offset
    if(wr_stat || write(pfd.fd, "-", 2) == -1) return -1;

    if (read(pfd.fd, readBuf, MAX_BUFFER_SIZE))
    {
        uint8_t offset = 0;
        for (int i = 0; i < 4; i++) {
            data[i] = (readBuf[offset+3] << 24) | (readBuf[offset+2] << 16) | (readBuf[offset+1] << 8) | readBuf[offset];
            offset+=4;
        }
    }

    read(pfd.fd, readBuf, MAX_BUFFER_SIZE);
    close(pfd.fd);

    return 0;
}

