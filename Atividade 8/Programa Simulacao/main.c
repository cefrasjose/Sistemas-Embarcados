#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// Definição dos Pinos 
#define POT_PIN ADC1_CHANNEL_3 // GPIO 4 
#define LED_PIN 45             // GPIO 45
#define BUTTON_PIN 46          // GPIO 46

void app_main() {
    // 1. Configuração do ADC
    adc1_config_width(ADC_WIDTH_BIT_12); 
    adc1_config_channel_atten(POT_PIN, ADC_ATTEN_DB_11); 

    // 2. Configuração do Timer PWM (LEDC)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_12_BIT, // 12 bits para casar com o ADC
        .freq_hz          = 1000,              
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // 3. Configuração do Canal PWM (LEDC)
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    // 4. Configuração do Botão
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY); // Usa o pull-up interno do pino 46

    // Variáveis de controle
    int adc_val = 0;
    bool frozen = false;
    int last_button_state = gpio_get_level(BUTTON_PIN); 

    while (1) {
        // Leitura do botão
        int current_button_state = gpio_get_level(BUTTON_PIN);

        // Detecta borda de descida (botão foi pressionado: de 1 para 0)
        if (last_button_state == 1 && current_button_state == 0) {
            frozen = !frozen; // Inverte o estado (congela/descongela)
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce
        }
        last_button_state = current_button_state;

        // Se não estiver congelado, continua lendo o ADC e alterando o brilho
        if (!frozen) {
            adc_val = adc1_get_raw(POT_PIN);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, adc_val);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }

        // Calcula a tensão para o terminal
        float voltage = (adc_val * 3.3) / 4095.0;

        // Imprime os valores
        printf("Raw ADC: %d | Tensao: %.2fV | Estado: %s\n", 
               adc_val, voltage, frozen ? "CONGELADO" : "LENDO");

        vTaskDelay(pdMS_TO_TICKS(100)); 
    }
}
