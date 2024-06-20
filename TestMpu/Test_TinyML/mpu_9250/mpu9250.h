#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#ifndef mpu9250_h
#define mpu9250_h

void mpu9250_reset();
uint8_t mpu9250_read_byte(uint8_t reg);
void read_acceleration(float *acceleration);
void start_i2c(uint8_t SDA, uint8_t SCL);

#endif