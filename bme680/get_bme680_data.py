#!/usr/bin/env python
# BME680 library via https://github.com/pimoroni/bme680-python
import bme680
import time
from pathlib import Path
# sudo pip install posix_ipc
import posix_ipc as ipc
import mmap
import struct
import signal

# An exit flaging class to enable cleanup on exit.
class ExitHandler:
  killed = False
  
  def __init__(self):
    # Catch these signals and set the killed flag
    # so that the main loop knows to stop.
    signal.signal(signal.SIGHUP, self.exit_gracefully)
    signal.signal(signal.SIGINT, self.exit_gracefully)
    signal.signal(signal.SIGQUIT, self.exit_gracefully)
    signal.signal(signal.SIGTERM, self.exit_gracefully)

  def exit_gracefully(self, *args):
    self.killed = True

if __name__ == '__main__':
  print(
    """Gathers BME680 data and makes it available to other programs via POSIX shared memory mapping.
    Press Ctrl+C to exit!""")
  processKiller = ExitHandler()

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
  # Setup gas heater to get air particulate readings.
  sensor.set_gas_status(bme680.ENABLE_GAS_MEAS)
  sensor.set_gas_heater_temperature(320)
  sensor.set_gas_heater_duration(150)
  sensor.select_gas_heater_profile(0)
  print("Finished setting up sensor.")

  # MMap configuration variables.
  NUM_DATA_POINTS = 4
  NUM_BYTES = 8 * NUM_DATA_POINTS
  SHARED_PATH = "/bme680_data"
  
  # Cleanup previous mappings and semaphores.
  memory = ipc.SharedMemory(SHARED_PATH, ipc.O_CREAT)
  memory.unlink()
  sharedSemaphore = ipc.Semaphore(SHARED_PATH, ipc.O_CREAT)
  sharedSemaphore.unlink()

  # Create and use new maps and semaphores.
  memory = ipc.SharedMemory(SHARED_PATH, ipc.O_CREX, size=NUM_BYTES)
  sharedMemMap = mmap.mmap(memory.fd, memory.size)
  memory.close_fd()
  sharedSemaphore = ipc.Semaphore(SHARED_PATH, ipc.O_CREX)
  sharedSemaphore.release()
  currentData = [0.0] * NUM_DATA_POINTS

  while not processKiller.killed:
    # Poll the sensor and get some data.
    if sensor.get_sensor_data():
      currentData[0] = round(sensor.data.temperature, 2)
      currentData[1] = round(sensor.data.humidity, 2)
      currentData[2] = round(sensor.data.pressure, 2)
      currentData[3] = round(sensor.data.gas_resistance, 2)
      print(f'{currentData[0]:.2f}, {currentData[1]:.2f}, {currentData[2]:.2f}, {currentData[3]:.2f}')

      sharedSemaphore.acquire()
      for i in range(NUM_DATA_POINTS):
        sharedMemMap.seek(i * 8)
        sharedMemMap.write(struct.pack('d', currentData[i]))
      sharedSemaphore.release()

    # The BME680 has a maximum polling rate of 1 second per
    # readout if the gas heater is in use.
    time.sleep(1)

  print("Closing...")
  memory.unlink()
  sharedMemMap.close()
  sharedSemaphore.unlink()
