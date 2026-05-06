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
    // 1. Configuração do ADC (3.3V)
    adc1_config_width(ADC_WIDTH_BIT_12); 
    adc1_config_channel_atten(POT_PIN, ADC_ATTEN_DB_11); 

    // 2. Configuração do Timer PWM (LEDC)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_12_BIT, 
        .freq_hz          = 1000, // 1kHz             
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
    // Mantemos o PULLUP_ONLY ativado no software por segurança no simulador, 
    // embora o esquemático agora exija um resistor externo de pull-up.
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY); 

    // Variáveis de controle de estado
    int adc_val = 0;
    bool hold_mode = false;
    int last_button_state = gpio_get_level(BUTTON_PIN); 
    
    // Variável para controle de tempo da impressão (não-bloqueante)
    TickType_t last_print_time = xTaskGetTickCount();

    while (1) {
        // --- LEITURA DO BOTÃO E CONTROLE DE ESTADO ---
        int current_button_state = gpio_get_level(BUTTON_PIN);

        // Detecta borda de descida (botão pressionado) para alternar o modo HOLD
        if (last_button_state == 1 && current_button_state == 0) {
            hold_mode = !hold_mode; 
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce do botão
        }
        last_button_state = current_button_state;

        // --- ATUALIZAÇÃO DO ADC E PWM ---
        // Se NÃO estiver em modo HOLD, atualiza a leitura e o brilho em tempo real
        if (!hold_mode) {
            adc_val = adc1_get_raw(POT_PIN);
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, adc_val);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }

        // --- IMPRESSÃO NO TERMINAL A CADA 500ms ---
        // Verifica se já se passaram 500ms desde a última impressão
        if ((xTaskGetTickCount() - last_print_time) >= pdMS_TO_TICKS(500)) {
            
            // Calcula a tensão em milivolts (mV) com base no valor máximo de 3300mV
            float voltage_mv = (adc_val * 3300.0) / 4095.0;

            // Imprime o valor bruto, a tensão em mV e o estado (LIVE ou HOLD)
            printf("Raw ADC: %d | Tensao: %.0f mV | Estado: %s\n", 
                   adc_val, voltage_mv, hold_mode ? "HOLD" : "LIVE");
            
            // Reseta o temporizador de impressão
            last_print_time = xTaskGetTickCount();
        }

        // Pequeno delay obrigatório para alimentar o watchdog do RTOS
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}
