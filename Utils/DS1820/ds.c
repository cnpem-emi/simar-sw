#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

double get_temp(FILE *f)
{
    char *buf = 0;
    long length;
    if (f)
    {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buf = malloc(length);
    }

    if (buf)
    {
        fread(buf, 1, length, f);
        buf = strstr(buf, "t=");
    }

    return atoi(buf + 2) / 1000.0;
}

char *find_device()
{
    DIR *dir;
    char *device_name = malloc(80);
    struct dirent *de;

    dir = opendir("/sys/bus/w1/devices/");
    while (dir)
    {
        de = readdir(dir);
        if (!de)
            break;

        if (strstr(de->d_name, "28") != NULL)
        {
            snprintf(device_name, 335, "/sys/bus/w1/devices/%s/w1_slave", de->d_name);
            break;
        }
    }
    closedir(dir);

    return device_name;
}

int main()
{
    FILE *f = fopen(find_device(), "r");
    while (1)
        printf("%.2f C\n", get_temp(f));

    fclose(f);
}
