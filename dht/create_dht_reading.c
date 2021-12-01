#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "get_dht_data.h"
#include "send_sensor_data.h"

/* The number of samples to take for averaging. */
#define NUM_SAMPLES 5

/* Return status values of the sensor read function. */
#define DHT_OK               0
#define DHT_ERROR_ARG       -1
#define DHT_ERROR_CHECKSUM  -2
#define DHT_ERROR_TIMEOUT   -3

/* The pin you have the sensor hanging off. */
uint32_t DHT_PIN;

int main(int argc, char *argv[]) {
  // Check for proper amount of arguments provided.
  if(argc != 8) {
    printf("Invalid number of args provided: %d\n", argc - 1);
    return 1;
  }

  // Grab the pin that we are using for the dht sensor from the arguments list.
  DHT_PIN = abs(strtoul(argv[7], NULL, 10));

  // Setup document insertion function details.
  char *db_credentials[5];
  for(int i = 1; i < 6; i++)
    db_credentials[i - 1] = argv[i];

  // Get the number of minutes to wait between sensor readings from the arguments.
  uint32_t n_minutes_per_datapoint = abs(strtoul(argv[6], NULL, 10));
  if(n_minutes_per_datapoint == 0) {
    printf("Invalid minutes_per_sample parameter, defaulting to 5 minute interval.\n");
    n_minutes_per_datapoint = 5;
  }

  // Create variables for data readout.
  int32_t humidity, temperature, humidity_decimal, temperature_decimal;
  int32_t samples[NUM_SAMPLES][4];

  // Setup time constructs.
  time_t seconds;
  seconds = time(&seconds);

  // Floor to closest n minutes.
  uint32_t closest = n_minutes_per_datapoint * 60;
  int64_t adjusted_time = (int64_t) ((int64_t)(seconds / closest) * closest) * 1000;

  // Setup wiringPi pins.
  wiringPiSetup();
  
  uint32_t samples_taken = 0;
  while(samples_taken < NUM_SAMPLES) {
    int32_t ret = get_dht_data(
      &DHT_PIN,
      &humidity,
      &temperature,
      &humidity_decimal,
      &temperature_decimal);
    
    if (ret == DHT_OK ) {
      // Record the sample.
      samples[samples_taken][0] = humidity;
      samples[samples_taken][1] = humidity_decimal;
      samples[samples_taken][2] = temperature;
      samples[samples_taken][3] = temperature_decimal;

      // Display the results.
      printf("Humidity: %.1f%% RH, Temperature: %.1f°C (%.f°F)\n",
          (double) (humidity + humidity_decimal * 0.1),
          (double) (temperature + temperature_decimal * 0.1),
          (double) ((temperature + temperature_decimal * 0.1) * 9.0/5.0) + 32);
          
      // Increment the counter.
      samples_taken++;
      delay(2500);
    }
    else if (ret == DHT_ERROR_CHECKSUM) {
      puts("Checksum error.");
      delay(2500);
    }
    else if (ret == DHT_ERROR_TIMEOUT) {
      puts("Timeout.");
      delay(2500);
    }
  }

  // If we have successfully collected the sample data, then write it out to file.
  if (samples_taken == NUM_SAMPLES) {
    puts("Completed sampling.");

    // Calculate the mean of temperature and humidity, then write the results to file.
    double mean_humidity = 0.0f;
    double mean_temperature = 0.0f;
    
    for(int i = 0; i < NUM_SAMPLES; i++) {
      // printf("%d.%d, %d.%d\n", samples[i][0], samples[i][1], samples[i][2], samples[i][3]);
      mean_humidity += (double) (samples[i][0] + samples[i][1] * 0.1);
      mean_temperature += (double) (samples[i][2] + samples[i][3] * 0.1);
    }

    mean_humidity /= NUM_SAMPLES;
    mean_temperature /= NUM_SAMPLES;

    printf("Average: %.1f, %.1f\n", mean_temperature, mean_humidity);

    send_sensor_data(&adjusted_time, &mean_temperature, &mean_humidity, db_credentials);
  }

  return 0;
}
