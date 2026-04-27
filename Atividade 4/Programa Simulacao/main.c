#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

// Definição dos Pinos
#define LED_BIT0 GPIO_NUM_4
#define LED_BIT1 GPIO_NUM_5
#define LED_BIT2 GPIO_NUM_6
#define LED_BIT3 GPIO_NUM_7

#define BTN_INC  GPIO_NUM_15
#define BTN_STEP GPIO_NUM_16

static const char *TAG = "CONTADOR_4BITS";

// Variáveis de estado
volatile uint8_t counter = 0;
volatile uint8_t step = 1;

// Variáveis para controle de Debounce (Tempo em microssegundos)
volatile int64_t last_isr_time_inc = 0;
volatile int64_t last_isr_time_step = 0;
const int64_t debounce_time_us = 200000; // 200ms

// Handle da Task principal para receber notificações
static TaskHandle_t main_task_handle = NULL;

// Função para atualizar o estado físico dos LEDs
void update_leds(uint8_t val) {
    gpio_set_level(LED_BIT0, (val >> 0) & 1);
    gpio_set_level(LED_BIT1, (val >> 1) & 1);
    gpio_set_level(LED_BIT2, (val >> 2) & 1);
    gpio_set_level(LED_BIT3, (val >> 3) & 1);
}

// Rotina de Serviço de Interrupção (ISR) para os botões
static void IRAM_ATTR btn_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    int64_t current_time = esp_timer_get_time();

    if (gpio_num == BTN_INC) {
        if (current_time - last_isr_time_inc > debounce_time_us) {
            // Aplica máscara 0x0F (0000 1111) para garantir o limite de 4 bits (0 a 15)
            counter = (counter + step) & 0x0F;
            last_isr_time_inc = current_time;
            
            // Notifica a task principal para atualizar os LEDs
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(main_task_handle, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
        }
    } 
    else if (gpio_num == BTN_STEP) {
        if (current_time - last_isr_time_step > debounce_time_us) {
            // Alterna o incremento entre 1 e 2
            step = (step == 1) ? 2 : 1;
            last_isr_time_step = current_time;
            
            // Notifica a task para atualizar o log
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(main_task_handle, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
        }
    }
}

void app_main(void) {
    main_task_handle = xTaskGetCurrentTaskHandle();

    // Configuração dos Pinos dos LEDs (Saída)
    gpio_config_t led_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL<<LED_BIT0) | (1ULL<<LED_BIT1) | (1ULL<<LED_BIT2) | (1ULL<<LED_BIT3),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&led_conf);

    // Configuração dos Pinos dos Botões (Entrada com Pull-up interno e interrupção na borda de descida)
    gpio_config_t btn_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<BTN_INC) | (1ULL<<BTN_STEP),
        .pull_down_en = 0,
        .pull_up_en = 1 // Ativa pull-up interno
    };
    gpio_config(&btn_conf);

    // Instala e associa o serviço de ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_INC, btn_isr_handler, (void*) BTN_INC);
    gpio_isr_handler_add(BTN_STEP, btn_isr_handler, (void*) BTN_STEP);

    // Estado inicial
    update_leds(counter);
    ESP_LOGI(TAG, "Sistema Iniciado. Contador: %d | Incremento: +%d", counter, step);

    while(1) {
        // A task fica bloqueada aqui, sem consumir CPU, até receber notificação da ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        update_leds(counter);
        ESP_LOGI(TAG, "Contador: %d (0x%X) | Incremento atual: +%d", counter, counter, step);
    }
}
