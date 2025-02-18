#include "ultrasonic.h"

static uint trig_pin;
static uint echo_pin;

void ultrasonic_init(uint tp, uint ep) {
    trig_pin = tp;
    echo_pin = ep;
    gpio_init(trig_pin);
    gpio_set_dir(trig_pin, GPIO_OUT);
    gpio_put(trig_pin, 0);

    gpio_init(echo_pin);
    gpio_set_dir(echo_pin, GPIO_IN);
}

float measure_distance() {
    // Inicia o trigger
    gpio_put(trig_pin, 1);  // Envia um pulso de 10us
    sleep_us(10);
    gpio_put(trig_pin, 0);

    // Aguarda o sinal de echo
    while (gpio_get(echo_pin) == 0);  // Espera o início do pulso de echo
    absolute_time_t start_time = get_absolute_time();  // Marca o tempo de início do pulso

    while (gpio_get(echo_pin) == 1);  // Aguarda o fim do pulso de echo
    absolute_time_t end_time = get_absolute_time();  // Marca o tempo do fim do pulso

    // Calcula a distância
    int64_t time_taken = absolute_time_diff_us(start_time, end_time);  // Tempo em microssegundos
    float distance = (time_taken / 2.0) * 0.0343;  // Distância em centímetros (340m/s é a velocidade do som)

    return distance;
}
