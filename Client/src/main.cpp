#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "ei_run_classifier.h"

#include "mpuObject.h"


//#define ssid "DIGI-S26z"
//#define pass "pR4TAmCwqN"
#define ssid "Redmi Note 9"
#define pass "1122334455"


#define SERVER_IP "192.168.144.5"
#define SERVER_PORT 4242

static struct tcp_pcb *tcp_pcb;
static bool connected = false;

static bool debug_nn = false;

mpu9250 mpu((uint8_t)4, (uint8_t)5);

static err_t on_connect(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Connection error\n");
        return err;
    }
    printf("Connected to server\n");
    connected = true;
    return ERR_OK;
}

static void send_message(struct tcp_pcb *tpcb, char* message) {
    err_t err = tcp_write(tpcb, message, strlen(message), TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        tcp_output(tpcb);
        printf("Message sent: %s\n", message);
    } else {
        printf("Error sending message: %d\n", err);
    }
}

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);


    //partea de trimitere date
    cyw43_arch_enable_sta_mode();
    while (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("failed to connect\n");
    }

    printf("Connected to Wi-Fi\n");

    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    const ip4_addr_t *ip = netif_ip4_addr(netif);
    printf("IP Address: %s\n", ip4addr_ntoa(ip));

    tcp_pcb = tcp_new();
    if (tcp_pcb == NULL) {
        printf("Error creating PCB\n");
        return 1;
    }

    ip4_addr_t server_ip;
    ip4addr_aton(SERVER_IP, &server_ip);

    tcp_connect(tcp_pcb, &server_ip, SERVER_PORT, on_connect);

    //partea de achizitie si clasificare a datelor
    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3) {
        ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return 1;
    }


    while (true) {
        cyw43_arch_poll();

        ei_printf("\nStarting inferencing in 1 seconds...\n");
        sleep_ms(1000);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
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

        float max = 0;
        char state[20];
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if(outputData.classification[ix].value > max)
            {
                max = outputData.classification[ix].value;
                strcpy(state, outputData.classification[ix].label);
            }
        }

        ei_printf("    %s: %.5f\n", state, max);
        #if EI_CLASSIFIER_HAS_ANOMALY == 1
            ei_printf("    anomaly score: %.3f\n", outputData.anomaly);
        #endif
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        char message[50] = "Stanga -> ";
        strcat(message, state);

        if (connected) {
            send_message(tcp_pcb, message);
        }
    }

    return 0;
}