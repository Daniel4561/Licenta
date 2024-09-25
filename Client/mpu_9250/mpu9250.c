#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "mpu9250.h"
#include "pico/binary_info.h"

#define I2C_PORT i2c0
#define MPU9250_ADDR 0x68

// MPU9250 registers
#define WHO_AM_I 0x75
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
void mpu9250_reset() {
    uint8_t buf[] = {PWR_MGMT_1, 0x00};
    i2c_write_blocking(I2C_PORT, MPU9250_ADDR, buf, 2, false);
}

uint8_t mpu9250_read_byte(uint8_t reg) {
    uint8_t val;
    i2c_write_blocking(I2C_PORT, MPU9250_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU9250_ADDR, &val, 1, false);
    return val;
}

void read_acceleration(float *acceleration) {
    uint8_t buffer[6];
    uint8_t reg = ACCEL_XOUT_H;
    i2c_write_blocking(I2C_PORT, MPU9250_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU9250_ADDR, buffer, 6, false);

    int16_t raw_ax = (buffer[0] << 8 | buffer[1]);
    int16_t raw_ay = (buffer[2] << 8 | buffer[3]);
    int16_t raw_az = (buffer[4] << 8 | buffer[5]);

    acceleration[0] = raw_ax / 16384.0f;
    acceleration[1] = raw_ay / 16384.0f;
    acceleration[2] = raw_az / 16384.0f;
}

void start_i2c(uint8_t SDA, uint8_t SCL)
{
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_pull_up(SCL);

    mpu9250_reset();

    uint8_t who_am_i = mpu9250_read_byte(WHO_AM_I);
    printf("MPU9250 WHO_AM_I: 0x%02X\n", who_am_i);
}