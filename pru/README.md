# Usage

## Installation (Python)
```sh
cd library/Python && python3 setup.py install
```

## Counting (Python)
```python
from count_pru import count_pru
count_pru.count_pru(1000000, 0) # 1 second, PRU 0
```

## Counting (C)
```c
#include <stdio.h>
#include <stdint.h>
#include "count.h"

int main(void)
{
    uint32_t data[4];
    count_pru(1000000, 0, data); // 1 second, PRU 0

    for(int i = 0; i < 4; i++) printf("Count for %d: %d\n", i, data[i]);
    return 0;
}
```
