#include <stdio.h>
#include "mpuObject.h"
#include "pico/stdlib.h"
#include "ei_run_classifier.h"
#include "hardware/gpio.h"


static bool debug_nn = false;
const uint LED_PIN = 25;

mpu9250 mpu((uint8_t)4, (uint8_t)5);

int main()
{
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return 1;
    }
    
    while (true){
        
        ei_printf("\nStarting inferencing in 2 seconds...\n");
        sleep_ms(2000);
        gpio_put(LED_PIN, 1);
        ei_printf("Sampling...\n");

        float inputBuffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = { 0 }; // buffer-ul in care citim valorile

        for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3) {
            uint64_t next_tick = ei_read_timer_us() + (EI_CLASSIFIER_INTERVAL_MS * 1000);
            
            mpu.updateData();

            // citirea valorilor propriu-zise
            inputBuffer[ix] = mpu.acceleration[0];
            inputBuffer[ix + 1] = mpu.acceleration[1];
            inputBuffer[ix + 2] = mpu.acceleration[2];

            sleep_us(next_tick - ei_read_timer_us());
        }

        // transformarea semnalului nostru pentru modelul antrenat
        signal_t edgeImpulseData;
        int err = numpy::signal_from_buffer(inputBuffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &edgeImpulseData);
        if (err != 0) {
            ei_printf("Failed to create signal from buffer (%d)\n", err);
            return 1;
        }

        // comparatia dintre input cu reteaua antrenata
        ei_impulse_result_t outputData = { 0 };

        err = run_classifier(&edgeImpulseData, &outputData, debug_nn);
        if (err != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", err);
            return 1;
        }

        // afisarea rezultatelor
        ei_printf("Predictions ");
        ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
            outputData.timing.dsp, outputData.timing.classification, outputData.timing.anomaly);
        ei_printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: %.5f\n", outputData.classification[ix].label, outputData.classification[ix].value);
        }
    #if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: %.3f\n", outputData.anomaly);
    #endif
    gpio_put(LED_PIN, 0);
    }

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
//#error "Invalid model for current sensor"
#endif
return 0;
}
