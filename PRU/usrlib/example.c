#include <stdio.h>
#include <stdint.h>
#include "cpru_userspace.h"

int main(void)
{
    uint32_t data[4];
    count_pru(1000000, data);

    for(int i = 0; i < 4; i++) printf("Count for %d: %d\n", i, data[i]);
    return 0;
}
