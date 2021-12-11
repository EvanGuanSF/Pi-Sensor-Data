#!/usr/bin/env python
import bme680
import time
from pathlib import Path
import fcntl

print("""read-all.py - Displays temperature, pressure, humidity, and gas.
Press Ctrl+C to exit!""")

try:
    sensor = bme680.BME680(bme680.I2C_ADDR_PRIMARY)
except (RuntimeError, IOError):
    sensor = bme680.BME680(bme680.I2C_ADDR_SECONDARY)

# These oversampling settings can be tweaked to
# change the balance between accuracy and noise in the data.
sensor.set_humidity_oversample(bme680.OS_2X)
sensor.set_pressure_oversample(bme680.OS_4X)
sensor.set_temperature_oversample(bme680.OS_8X)
sensor.set_filter(bme680.FILTER_SIZE_3)
sensor.set_gas_status(bme680.ENABLE_GAS_MEAS)

sensor.set_gas_heater_temperature(320)
sensor.set_gas_heater_duration(150)
sensor.select_gas_heater_profile(0)

# Up to 10 heater profiles can be configured, each
# with their own temperature and duration.
# sensor.set_gas_heater_profile(200, 150, nb_profile=1)
# sensor.select_gas_heater_profile(1)

try:
    data_file_name = str(Path.home()) + '/.pi_sensor_data/bme680_data.txt'
    
    while True:
        if sensor.get_sensor_data():
            # Possible race condition between reading and writing to/from the data file.
            # Try to get a lock on the data file and then write sensor data.
            with open(data_file_name, 'w') as data_file:
                fcntl.flock(data_file, fcntl.LOCK_EX)
                data_file.write('temperature_c:{0:.2f}\n'.format(sensor.data.temperature) +
                    'relative_humidity:{0:.2f}\n'.format(sensor.data.humidity) +
                    'pressure_hpa:{0:.2f}\n'.format(sensor.data.pressure) +
                    'gas_resistance_ohms:{0:.2f}\n'.format(sensor.data.gas_resistance))
                fcntl.flock(data_file, fcntl.LOCK_UN)
                data_file.close()

        time.sleep(1)

except KeyboardInterrupt:
    pass
