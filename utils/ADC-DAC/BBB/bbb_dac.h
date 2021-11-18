#ifndef DAC_H
#define DAC_H

// Kernel 4.19.94-ti-r59

#define P9_42 "/sys/class/pwm/pwmchip0/pwm-0:0/"

#define P9_22 "/sys/class/pwm/pwmchip1/pwm-1:0/"
#define P9_21 "/sys/class/pwm/pwmchip1/pwm-1:1/"

#define P9_14 "/sys/class/pwm/pwmchip4/pwm-4:0/"
#define P9_16 "/sys/class/pwm/pwmchip4/pwm-4:1/"

#define P8_19 "/sys/class/pwm/pwmchip7/pwm-7:0/"
#define P8_13 "/sys/class/pwm/pwmchip7/pwm-7:1/"

int set_period(int per, char* pin);
int set_duty(int dut, char* pin);
int enable(char* pin);

#endif
