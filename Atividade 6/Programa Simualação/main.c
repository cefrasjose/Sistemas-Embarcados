#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

// Definição dos pinos
#define LED_PIN GPIO_NUM_4
#define BUTTON_PIN GPIO_NUM_5

// Variáveis globais para controle das tarefas e timers
TaskHandle_t button_task_handle = NULL;
TimerHandle_t auto_off_timer = NULL;
bool led_state = false;

// 1. CALLBACK DO TEMPORIZADOR (10 Segundos)
void auto_off_callback(TimerHandle_t xTimer) {
    led_state = false;
    gpio_set_level(LED_PIN, 0);
    printf("Temporizador: LED desligado apos 10s de inatividade.\n");
}

// 2. ROTINA DE SERVIÇO DE INTERRUPÇÃO (ISR)
// A ISR deve ser o mais rápida possível. Ela não usa vTaskDelay ou printf.
// Seu único trabalho é enviar uma notificação para acordar a Task do botão.
void IRAM_ATTR button_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// 3. TAREFA DE PROCESSAMENTO DO BOTÃO
void button_task(void* arg) {
    while (1) {
        // A tarefa dorme (bloqueada, sem gastar CPU) até a ISR enviar a notificação
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Tratamento de Debounce por software (espera 50ms para estabilizar)
        vTaskDelay(pdMS_TO_TICKS(50));

        // Verifica se, após o debounce, o botão ainda está pressionado (nível LOW)
        if (gpio_get_level(BUTTON_PIN) == 0) {
            int hold_time = 50;
            bool forced_off = false;

            // Loop para medir quanto tempo o usuário segura o botão
            while (gpio_get_level(BUTTON_PIN) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
                hold_time += 10;

                // Requisito 3: Desligamento forçado após 2 segundos (2000 ms)
                if (hold_time >= 2000) {
                    forced_off = true;
                    led_state = false;
                    gpio_set_level(LED_PIN, 0);
                    xTimerStop(auto_off_timer, 0); // Cancela o timer de 10s
                    printf("Desligamento forcado: Botao segurado por 2s.\n");

                    // Trava de segurança: Espera o usuário soltar o botão antes de continuar
                    while(gpio_get_level(BUTTON_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break; // Sai do loop de medição
                }
            }

            // Se o botão foi solto antes de 2 segundos (Clique Normal / Subsequente)
            if (!forced_off) {
                // Requisitos 1 e 2: Liga o LED e inicia/reinicia o temporizador de 10s
                led_state = true;
                gpio_set_level(LED_PIN, 1);
                
                // xTimerStart também serve para resetar o timer se ele já estiver rodando
                xTimerStart(auto_off_timer, 0); 
                printf("Clique detectado: LED ligado / Temporizador de 10s renovado.\n");
            }
        }
        
        // Limpa notificações extras ("bounces") que possam ter ocorrido enquanto a task processava
        ulTaskNotifyTake(pdTRUE, 0);
    }
}

// 4. FUNÇÃO PRINCIPAL
void app_main(void) {
    // Configuração do LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);

    // Configuração do Botão
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
    
    // Configura a interrupção para disparar na borda de descida (quando pressionado)
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE); 

    // Cria o temporizador de software do FreeRTOS (10000 ms, modo One-Shot)
    auto_off_timer = xTimerCreate("AutoOffTimer", pdMS_TO_TICKS(10000), pdFALSE, (void*)0, auto_off_callback);

    // Cria a Task que vai gerenciar a lógica do botão
    xTaskCreate(button_task, "Button Task", 2048, NULL, 10, &button_task_handle);

    // Instala o serviço global de interrupções e vincula o pino à função ISR
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
    
    printf("Sistema Iniciado com Interrupcoes. Aguardando eventos...\n");
}
