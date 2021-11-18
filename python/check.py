import Adafruit_BBIO.GPIO as GPIO
import board
from adafruit_bme280 import basic as adafruit_bme280
import adafruit_bmp280

from time import sleep

i2cAd0 = "P9_15"
i2cAd1 = "P9_16"

# ---------- Set Pinouts ----------
GPIO.setup(i2cAd0, GPIO.OUT)
GPIO.output(i2cAd0, GPIO.LOW)

GPIO.setup(i2cAd1, GPIO.OUT)
GPIO.output(i2cAd1, GPIO.LOW)
# ---------------------------------


def chooseI2Cchannel(ch):

    if ch == 0:
        GPIO.output(i2cAd0, GPIO.LOW)
        GPIO.output(i2cAd1, GPIO.LOW)
    if ch == 1:
        GPIO.output(i2cAd0, GPIO.HIGH)
        GPIO.output(i2cAd1, GPIO.LOW)
    if ch == 2:
        GPIO.output(i2cAd0, GPIO.LOW)
        GPIO.output(i2cAd1, GPIO.HIGH)
    if ch == 3:
        GPIO.output(i2cAd0, GPIO.HIGH)
        GPIO.output(i2cAd1, GPIO.HIGH)
    sleep(1)


i2c = board.I2C()

while True:
    for i in range(4):
        chooseI2Cchannel(i)
        for addr in [0x76, 0x77]:
            try:
                bme280 = adafruit_bme280.Adafruit_BME280_I2C(i2c, address=addr)
                print(
                    "Found BME sensor at channel {}, address {}. Pressure reading: {}".format(
                    i,
                    addr,
                    bme280.pressure)
                )
            except:
                try:
                    bmp280 = adafruit_bmp280.Adafruit_BMP280_I2C(i2c, address=addr)
                    print(
                        "Found BMP sensor at channel {}, address {}. Pressure reading: {}".format(
                        i,
                        addr,
                        bmp280.pressure)
                    )
                except Exception as e:
                    print(e)
                    print("Nothing on channel {}, address {}".format(i, addr))
        sleep(1)
