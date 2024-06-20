#include "pico/stdlib.h"

#ifndef mpuObject_h
#define mpuObject_h

class mpu9250
{
    public: 
    float acceleration[3];

    mpu9250(uint8_t SDA, uint8_t SCL);
    void updateData();
    void printData();
};

#endif