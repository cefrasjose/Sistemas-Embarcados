#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

// Definição dos Pinos
#define POT_CHAN ADC_CHANNEL_3 // GPIO 4
#define LED_PIN 45             
#define BUTTON_PIN 46          

void app_main() {
    //1. Configuração do Novo Driver ADC (One-shot)
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12, // Faixa de tensão
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, POT_CHAN, &config));

    //2. Configuração do Timer PWM (LEDC)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_12_BIT, 
        .freq_hz          = 1000,             
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    //3. Configuração do Canal PWM (LEDC)
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

    //4. Configuração do Botão
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY); 

    int adc_val = 0;
    bool hold_mode = false;
    int last_button_state = gpio_get_level(BUTTON_PIN); 
    TickType_t last_print_time = xTaskGetTickCount();

    while (1) {
        //Leitura do botão para alternar HOLD/LIVE
        int current_button_state = gpio_get_level(BUTTON_PIN);
        if (last_button_state == 1 && current_button_state == 0) {
            hold_mode = !hold_mode; 
            vTaskDelay(pdMS_TO_TICKS(50)); 
        }
        last_button_state = current_button_state;

        //Se NÃO estiver em HOLD, lê o ADC usando o novo driver
        if (!hold_mode) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, POT_CHAN, &adc_val));
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, adc_val);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }

        //Monitoramento a cada 500ms
        if ((xTaskGetTickCount() - last_print_time) >= pdMS_TO_TICKS(500)) {
            float voltage_mv = (adc_val * 3300.0) / 4095.0;
            printf("Raw ADC: %d | Tensao: %.0f mV | Estado: %s\n", 
                   adc_val, voltage_mv, hold_mode ? "HOLD" : "LIVE");
            last_print_time = xTaskGetTickCount();
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}
