#pragma once
#include <stdint.h>

int get_dht_data(int32_t *pin, int32_t *humidity, int32_t *temperature, int32_t* humidity_integral, int32_t* temperature_integral);