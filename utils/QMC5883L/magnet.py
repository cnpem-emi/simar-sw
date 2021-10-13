import py_qmc5883l
import redis
from time import sleep

sensor = py_qmc5883l.QMC5883L(i2c_bus=2)
r = redis.Redis(host="10.0.38.46", port=6379, db=0)

while True:
    values = sensor.get_magnet_raw()
    r.set("magnet_x", values[0])
    r.set("magnet_y", values[1])
    r.set("magnet_z", values[2])
    sleep(1)
