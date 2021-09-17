# DHT communication

Currently available:
- int bbb_dht_read(int type, Pin pin, float *humidity, float *temperature)
    - Reads DHT temperature. Currently supports AM2302, DHT11 and DHT22 sensors.
        - type: type of sensor (DHT11, DHT22, AM2302)
        - pin: data pin of the sensor
        - humidity: pointer to humidity float
        - temperature: pointer to temperature float

Notes: Heavily based on https://github.com/ChadLefort/beaglebone-dht by Chad Lefort/Tony DiCola. Just adjusted some things to conform to the same patterns and standards as other libs
