#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

//Configurações

//Define o tempo de atualização para cada passo das fases 
#define DELAY_MS 500

//Define os pinos GPIO conectados aos LEDs
#define LED1_GPIO 2  // Bit 0 (LSB - menos significativo)
#define LED2_GPIO 4  // Bit 1
#define LED3_GPIO 5  // Bit 2
#define LED4_GPIO 18 // Bit 3 (MSB - mais significativo)

//Array com os pinos dos LEDs para facilitar o acesso em loops
static const gpio_num_t led_gpios[] = {LED1_GPIO, LED2_GPIO, LED3_GPIO, LED4_GPIO};
const int NUM_LEDS = sizeof(led_gpios) / sizeof(led_gpios[0]);

//Funções de Controle dos LEDs

/*Configura os pinos GPIO dos LEDs como saídas digitais.
 */
void configure_leds(void) {
    gpio_config_t io_conf;
    //Desabilita interrupções
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //Configura como modo de saída 
    io_conf.mode = GPIO_MODE_OUTPUT;
    //Bit mask dos pinos a serem configurados
    io_conf.pin_bit_mask = ((1ULL << LED1_GPIO) | (1ULL << LED2_GPIO) | (1ULL << LED3_GPIO) | (1ULL << LED4_GPIO));
    //Desabilita pull-down
    io_conf.pull_down_en = 0;
    //Desabilita pull-up
    io_conf.pull_up_en = 0;
    //Aplica a configuração
    gpio_config(&io_conf);
}

/*Desliga todos os LEDs.*/
void turn_all_leds_off(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(led_gpios[i], 0);
    }
}

/*Implementa a Fase 1: Contador Binário.
  Os LEDs exibem valores binários de 0 a 15.
 */
void run_binary_counter_phase(void) {
    printf("Iniciando Fase 1: Contador Binário\n");
    for (int i = 0; i <= 15; i++) {
        //Usa operações de bitwise para extrair cada bit do contador 'i'
        //e atribui ao LED correspondente.
        gpio_set_level(LED1_GPIO, (i & 1) >> 0); // Bit 0
        gpio_set_level(LED2_GPIO, (i & 2) >> 1); // Bit 1
        gpio_set_level(LED3_GPIO, (i & 4) >> 2); // Bit 2
        gpio_set_level(LED4_GPIO, (i & 8) >> 3); // Bit 3

        // Aguarda o tempo definido [cite: 29]
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
    }
}

/* Implementa a Fase 2: Sequência de Varredura. 
   Os LEDs acendem em sequência e depois retornam
 */
void run_scan_sequence_phase(void) {
    printf("Iniciando Fase 2: Sequência de Varredura\n");

    // Sequência de ida (LED1 -> LED2 -> LED3 -> LED4) 
    for (int i = 0; i < NUM_LEDS; i++) {
        gpio_set_level(led_gpios[i], 1);
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(led_gpios[i], 0);
    }

    //Sequência de volta (LED4 -> LED3 -> LED2 -> LED1) 
    //Começa do penúltimo para não repetir o último
    for (int i = NUM_LEDS - 2; i >= 0; i--) {
        gpio_set_level(led_gpios[i], 1);
        vTaskDelay(DELAY_MS / portTICK_PERIOD_MS);
        gpio_set_level(led_gpios[i], 0);
    }
}


//Função Principal

void app_main(void) {
    //Configura os pinos dos LEDs
    configure_leds();

    //Garante que todos os LEDs iniciem apagados 
    turn_all_leds_off();
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Atraso inicial

    //Loop infinito para alternar entre as fases
    while (1) {
        //Executa a fase do contador binário
        run_binary_counter_phase();

        //Garante que os LEDs estejam apagados antes da próxima fase
        turn_all_leds_off();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        //Executa a fase da sequência de varredura
        run_scan_sequence_phase();
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
