#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

// Definição dos pinos
#define LED_PIN GPIO_NUM_4
#define BUTTON_PIN GPIO_NUM_5

// Tempos (em milissegundos)
#define DEBOUNCE_DELAY_MS 50
#define AUTO_OFF_DELAY_MS 10000

void app_main(void) {
    // 1. Configuração do LED (Saída)
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); // Inicia desligado

    // 2. Configuração do Botão (Entrada com Pull-up interno)
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

    // Variáveis de estado do sistema
    bool led_state = false;
    int button_state = 1;      // Estado atual estabilizado
    int last_reading = 1;      // Última leitura instantânea do pino
    
    // Variáveis de controle de tempo (usando os Ticks do FreeRTOS)
    uint32_t last_debounce_time = 0;
    uint32_t led_on_time = 0;

    // Loop infinito (Polling)
    while (1) {
        // Pega o tempo atual do sistema em milissegundos
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Lê o estado atual do pino do botão
        int reading = gpio_get_level(BUTTON_PIN);

        // --- TRATAMENTO DE DEBOUNCE ---
        // Se a leitura mudou (devido a ruído ou clique), reseta o temporizador de debounce
        if (reading != last_reading) {
            last_debounce_time = current_time;
        }

        // Se o estado permaneceu estável pelo tempo de debounce (50ms)
        if ((current_time - last_debounce_time) > DEBOUNCE_DELAY_MS) {
            
            // Se o estado estabilizado for diferente do último estado registrado
            if (reading != button_state) {
                button_state = reading;

                // Como usamos Pull-up, o clique é registrado no nível BAIXO (0)
                if (button_state == 0) {
                    // TOGGLE: Inverte o estado do LED
                    led_state = !led_state;
                    gpio_set_level(LED_PIN, led_state);

                    // Se o LED acabou de ser ligado, marca o momento (timestamp)
                    if (led_state) {
                        led_on_time = current_time;
                        printf("LED Ligado. Temporizador de 10s iniciado.\n");
                    } else {
                        printf("LED Desligado manualmente.\n");
                    }
                }
            }
        }
        
        // Atualiza a última leitura para a próxima iteração
        last_reading = reading;


        // --- TEMPORIZADOR DE SEGURANÇA (10 SEGUNDOS) ---
        // Se o LED estiver ligado E tiverem se passado 10000ms desde que ligou
        if (led_state && (current_time - led_on_time) >= AUTO_OFF_DELAY_MS) {
            led_state = false;
            gpio_set_level(LED_PIN, led_state);
            printf("Tempo limite atingido. LED desligado por seguranca.\n");
        }

        // Pequeno atraso para liberar processamento para o FreeRTOS e evitar acionamento do Watchdog
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
