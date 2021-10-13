#include <stdio.h>
#include "dht.h"
#include "../../SPI/common.h"


int main()
{
    double hum, tem;
    int tries = 10;

    gpio_t pin = {.pin = P8_11};

    while (tries)
    {
        if (bbb_dht_read(DHT11, pin, &hum, &tem) == 0)
        {
            printf("Temperature: %.2f\nHumidity: %.2f\n", tem, hum);
            break;
        }
        else
        {
            printf("Retrying... %d tries left\n", tries--);
        }
    }
}
