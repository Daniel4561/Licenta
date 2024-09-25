#include "mpuObject.h"
#include <stdio.h>
#include "pico/stdlib.h"

mpu9250 mpu(4, 5);  // Creates an mpu object

int main()
{
    stdio_init_all();

    sleep_ms(1000); 

    while (1)
    {
        mpu.updateData();

        printf(" %f, %f, %f\n", mpu.acceleration[0], mpu.acceleration[1], mpu.acceleration[2]);
        sleep_ms(10);

    }

    return 0;
}
