#include "mpuObject.h"
extern "C" {
    #include "mpu9250.h"
}

mpu9250::mpu9250(uint8_t SDA, uint8_t SCL)
{
    start_i2c(SDA, SCL);
    read_acceleration(acceleration);
}

void mpu9250::updateData()
{
    read_acceleration(acceleration);
}

void mpu9250::printData()
{
    printf("Acc. X = %f, Y = %f, Z = %f\n", acceleration[0], acceleration[1], acceleration[2]);
}