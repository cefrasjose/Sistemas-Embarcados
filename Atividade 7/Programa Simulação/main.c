#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

// Definições dos Pinos
#define TX2_PIN 17
#define RX2_PIN 16
#define LED_PIN 4

// Definições da UART
#define UART_PORT_NUM UART_NUM_2
#define BUF_SIZE 1024

void app_main(void)
{
    // 1. Configuração da UART2 (115200, 8 bits, 1 stop bit, sem paridade)
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    
    // Aplica a configuração
    uart_param_config(UART_PORT_NUM, &uart_config);
    // Configura os pinos TX e RX
    uart_set_pin(UART_PORT_NUM, TX2_PIN, RX2_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Instala o driver da UART
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);

    // 2. Configuração do GPIO do LED
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0); // Garante que inicia apagado

    uint8_t rx_buffer[BUF_SIZE];
    bool ligar_led = true; // Variável de controle para alternar o estado

    printf("Iniciando Simulacao UART Loopback...\n");
    printf("---------------------------------------------------\n");

    while (1) {
        // 3. Define a mensagem a ser enviada
        const char* msg_tx = ligar_led ? "LIGAR" : "DESLIGAR";
        
        // Envia a string pela UART2
        uart_write_bytes(UART_PORT_NUM, msg_tx, strlen(msg_tx));
        
        // Imprime o feedback de envio no console principal com printf
        printf("[TX] Enviado: %s\n", msg_tx);

        // Delay de 100ms para dar tempo ao sinal elétrico percorrer o jumper e ser lido
        vTaskDelay(pdMS_TO_TICKS(100));

        // 4. Processamento da Recepção
        int len = uart_read_bytes(UART_PORT_NUM, rx_buffer, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        
        if (len > 0) {
            rx_buffer[len] = '\0'; // Adiciona terminador nulo para transformar o array de bytes em string
            
            // Imprime o dado bruto que chegou na porta serial
            printf("[RX] Recebido na RX: %s\n", (char*)rx_buffer);

            // Verifica as condições e imprime o feedback da ação executada
            if (strncmp((char*)rx_buffer, "LIGAR", 5) == 0) {
                gpio_set_level(LED_PIN, 1);
                printf("-> STATUS: O codigo entrou no IF. O LED foi LIGADO!\n");
                
            } else if (strncmp((char*)rx_buffer, "DESLIGAR", 8) == 0) {
                gpio_set_level(LED_PIN, 0);
                printf("-> STATUS: O codigo entrou no ELSE IF. O LED foi DESLIGADO!\n");
                
            } else {
                printf("-> AVISO: Comando recebido nao reconhecido.\n"); 
            }
        } else {
            printf("-> ERRO: Nenhum dado recebido. Verifique se o jumper (fio azul) esta conectado!\n");
        }

        // Adiciona um separador visual no log para facilitar a leitura no terminal
        printf("---------------------------------------------------\n");

        // Alterna a lógica para a próxima execução ("LIGAR" -> "DESLIGAR" -> "LIGAR")
        ligar_led = !ligar_led;

        // Aguarda mais 1900ms para completar o ciclo exato de 2 segundos de envio periódico
        vTaskDelay(pdMS_TO_TICKS(1900)); 
    }
}
