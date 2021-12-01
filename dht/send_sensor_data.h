#pragma once
#include <stdint.h>

int send_sensor_data(int64_t *timestamp, double *temperature, double *humidity, char *db_credentials[]);