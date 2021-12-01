#include <wiringPi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

/* Return status values of the sensor read function. */
#define DHT_OK               0
#define DHT_ERROR_ARG       -1
#define DHT_ERROR_CHECKSUM  -2
#define DHT_ERROR_TIMEOUT   -3

/** 
 * How long to spin, waiting for input.
 */
#define DHT_MAXCOUNT 35000

/**
 * Number of bit pulses to expect from the DHT module.  Note that this is 41 because
 * the first pulse is a constant 50 microsecond pulse, with 40 pulses to
 * represent the data afterwards.
 */
#define DHT_PULSES	41

void sleep_millisec(uint32_t millis) {
  struct timespec sleep;
  sleep.tv_sec = millis / 1000;
  sleep.tv_nsec = (millis % 1000) * 1000000L;
  while (clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, &sleep) && errno == EINTR);
}

int get_dht_data(int32_t *pin, int32_t *humidity, int32_t *temperature, int32_t* humidity_decimal, int32_t* temperature_decimal) {
	// Make sure output pointers are ok.
  if (humidity == NULL || temperature == NULL ) {
    return DHT_ERROR_ARG;
  }

  *humidity = 0.0f;
  *temperature = 0.0f;

  // Array to store length of low and high pulses from the sensor.
  int pulseWidths[DHT_PULSES*2] = {0};

  // Signal sensor to output its data. High for ~50ms then low for ~20ms.
  pinMode(*pin, OUTPUT);
  digitalWrite(*pin, HIGH);
  sleep_millisec(50);
  digitalWrite(*pin, LOW);
  sleep_millisec(20);

  // Time the pulses coming in.
  pinMode(*pin, INPUT);
  // Tiny delay to let pin stabilise as input pin and let voltage come up
  // for(volatile int i=0; i<50; i++);

  // Wait for HIGH->LOW edge
  uint32_t count = 0;
  while(digitalRead(*pin)) {
    if (++count > DHT_MAXCOUNT) {
      return DHT_ERROR_TIMEOUT;
    }
  }
  
  // Record pulse widths
  int pulse=0;
  while (pulse < DHT_PULSES * 2) {
    // Time low
    while(!digitalRead(*pin)) {
      if (++pulseWidths[pulse] > DHT_MAXCOUNT) {
        return DHT_ERROR_TIMEOUT;
      }
    }
    ++pulse;
    // Time high
    while(digitalRead(*pin)) {
      if (++pulseWidths[pulse] > DHT_MAXCOUNT) {
        return DHT_ERROR_TIMEOUT;
      }
    }
    ++pulse;
  }

  // Convert pulse widths to bits and bytes
  int16_t bytes[5] = {0};
  uint8_t bit = 0;
  pulse = 2; // Skip over initial bit
  while (pulse < DHT_PULSES * 2) {
    bytes[bit>>3] <<= 1;
    if (pulseWidths[pulse] < pulseWidths[++pulse] ) {
      // High part is longer than the preceding low, so this bit is a 1. 
      bytes[bit>>3] |= 1;
    }
    // Otherwise high part is shorter, this bit is a 0.
    ++bit;
    ++pulse;
  }

  // Check the checksum
  if (bytes[4] != ((bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xff)) {
    return DHT_ERROR_CHECKSUM;
  }

  // Put data in the array.
  *humidity = (int32_t)bytes[0];
  *humidity_decimal = (int32_t)bytes[1];
  *temperature = (int32_t)bytes[2];
  *temperature_decimal = (int32_t)bytes[3];

  return DHT_OK;
}