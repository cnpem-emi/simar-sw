import time
import board

import digitalio
import busio
import adafruit_bmp280

import redis
import Adafruit_BBIO.GPIO as GPIO


class Door:
    def __init__(self):
        self.window = []
        self.strikes_closed = 0
        self.is_open = 0
        self.average = 0
        self.open_average = 0

    def get_open_iter(self, pressure):
        if self.average - pressure < -0.23 or (
            self.open_average != 0 and self.open_average - pressure < 0.23
        ):
            if self.strikes_closed > 9:
                self.is_open = 1
            elif self.is_open:
                self.strikes_closed = 10
            else:
                self.strikes_closed += 1
        else:
            if self.strikes_closed < 1:
                self.is_open = self.open_average = 0
            else:
                self.strikes_closed -= 1

    def update_open(self, new_val, past_pres):
        cache = []
        cache = self.window.copy()

        for i in range(9, 0, -1):
            self.window[i - 1] = cache[i]

        self.window[9] = new_val
        sum = 0

        for win_val in self.window:
            sum += win_val

        diff = self.average - new_val
        open_diff = self.open_average - new_val

        if self.average == 0 or (diff > -0.2 and diff < 0.4 and not self.is_open):
            self.average = sum / 10
        elif self.is_open and (
            self.open_average == 0 or (open_diff > -0.4 and open_diff < 0.2)
        ):
            self.open_average = sum / 10

        self.get_open_iter(new_val)

    def populate_window(self, pres):
        past_pres = 0
        for i in range(10):
            if past_pres == 0 or abs(past_pres - pres) < past_pres / 7:
                self.window.append(pres)
                past_pres = pres

        return past_pres


class Sensor:
    def __init__(
        self,
        redis_server="127.0.0.1",
        i2c_addr=0x76,
        use_i2c=True,
        t_key="temperature",
        p_key="pressure",
        d_key="is_open",
        cs_pin=board.P9_17,
    ):
        self.r = redis.Redis(host=redis_server, port=6379, db=0)
        self.t_key = t_key
        self.p_key = p_key
        self.d_key = d_key

        self.past_pres = 0
        if use_i2c:
            i2c = busio.I2C(board.SCL, board.SDA)
            self.bmp280 = adafruit_bmp280.Adafruit_BMP280_I2C(i2c, i2c_addr)
        else:
            spi = busio.SPI(board.SCK, board.MOSI, board.MISO)
            bmp_cs = digitalio.DigitalInOut(cs_pin)

            self.bmp280 = adafruit_bmp280.Adafruit_BMP280_SPI(spi, bmp_cs)
        self.bmp280.sea_level_pressure = 1013.25

        self.door = Door()
        self.past_pres = self.door.populate_window(self.read()["pressure"])

    def read(self):
        try:
            pres, temp = self.bmp280.pressure, self.bmp280.temperature
        except OSError:
            return False

        print(self.past_pres)
        if self.past_pres == 0 or abs(self.past_pres - pres) < self.past_pres / 7:
            if self.door.window:
                self.door.update_open(pres, self.past_pres)

            self.r.set(self.t_key, format(temp, ".3f"))
            self.r.set(self.p_key, format(pres, ".3f"))
            self.r.set(self.d_key, self.door.is_open)

            self.past_pres = pres

        return {"pressure": pres, "temperature": temp}


if __name__ == "__main__":
    GPIO.setup("P9_16", GPIO.OUT)
    GPIO.setup("P9_15", GPIO.OUT)

    GPIO.output("P9_16", GPIO.LOW)
    GPIO.output("P9_15", GPIO.HIGH)
    time.sleep(1)
    sensor = Sensor()
    GPIO.output("P9_16", GPIO.HIGH)
    time.sleep(1)
    sensor3 = Sensor(t_key="temperature_ext_cor", p_key="pressure_ext_cor", d_key="n")
    GPIO.output("P9_15", GPIO.LOW)
    time.sleep(1)
    sensor2 = Sensor(t_key="temperature_ext", p_key="pressure_ext", d_key="n")

    while True:
        GPIO.output("P9_16", GPIO.LOW)
        GPIO.output("P9_15", GPIO.HIGH)
        print("Sensor 1")
        time.sleep(1)
        print(sensor.read())

        GPIO.output("P9_16", GPIO.HIGH)
        print("Sensor 3")
        time.sleep(1)
        print(sensor3.read())

        GPIO.output("P9_15", GPIO.LOW)
        print("Sensor 2")
        time.sleep(1)
        print(sensor2.read())

        time.sleep(1)
