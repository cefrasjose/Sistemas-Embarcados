#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h" 

//CONFIGURAÇÃO DE HARDWARE
#define I2C_MASTER_SDA_IO    20    
#define I2C_MASTER_SCL_IO    21    
#define I2C_MASTER_FREQ_HZ   100000 
#define I2C_MASTER_NUM       I2C_NUM_0

#define POT_ADC_UNIT         ADC_UNIT_1
#define POT_ADC_CHAN         ADC_CHANNEL_3  // GPIO 4
#define LED_PIN              15             // GPIO 15
#define BTN_PIN              16             // GPIO 16

#define MPU6050_ADDR         0x68           
#define SENSITIVITY_2G       16384.0        

// --- ESTRUTURAS E GLOBAIS ---
typedef struct {
    float x, y, z;
} imu_data_t;

QueueHandle_t xPotQueue;
SemaphoreHandle_t xBtnSemaphore;
SemaphoreHandle_t xImuMutex;
adc_oneshot_unit_handle_t adc1_handle;

imu_data_t shared_imu = {0};
bool is_hold = false;

// Variáveis para alimentar o Console sem interferir na Fila
volatile int shared_pot_raw = 0;
volatile int shared_led_pct = 0;

//INICIALIZAÇÃO
void init_hw() {
    // Inicialização do I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // Inicialização do PWM (LED)
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_13_BIT, 
        .freq_hz = 5000,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK 
    };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel = {
        .gpio_num = LED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0
    };
    ledc_channel_config(&ledc_channel);

    // Inicialização do ADC
    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = POT_ADC_UNIT };
    adc_oneshot_new_unit(&init_config1, &adc1_handle);
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12, 
    };
    adc_oneshot_config_channel(adc1_handle, POT_ADC_CHAN, &config);

    // Inicialização do Botão
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
}

//TAREFAS RTOS
void task_potentiometer(void *pv) {
    int raw_val;
    while(1) {
        if (adc_oneshot_read(adc1_handle, POT_ADC_CHAN, &raw_val) == ESP_OK) {
            shared_pot_raw = raw_val; 
            
            // Mapeamento de 12 bits para 13 bits (Multiplicação por 2)
            uint32_t pwm_val = (uint32_t)raw_val * 2; 
            xQueueSend(xPotQueue, &pwm_val, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_led_control(void *pv) {
    uint32_t duty_cycle = 0;
    while(1) {
        if (xSemaphoreTake(xBtnSemaphore, 0) == pdTRUE) {
            is_hold = !is_hold;
        }

        if (xQueueReceive(xPotQueue, &duty_cycle, pdMS_TO_TICKS(50))) {
            if (!is_hold) { 
                ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                
                shared_led_pct = (duty_cycle * 100) / 8191;
            }
        }
    }
}

void task_button(void *pv) {
    while(1) {
        if (gpio_get_level(BTN_PIN) == 0) {
            xSemaphoreGive(xBtnSemaphore);
            vTaskDelay(pdMS_TO_TICKS(300)); 
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void task_imu(void *pv) {
    uint8_t data[6];
    uint8_t reg = 0x3B; 
    
    while(1) {
        i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_ADDR, &reg, 1, data, 6, pdMS_TO_TICKS(100));
        
        int16_t raw_x = (data[0] << 8) | data[1];
        int16_t raw_y = (data[2] << 8) | data[3];
        int16_t raw_z = (data[4] << 8) | data[5];

        if (xSemaphoreTake(xImuMutex, portMAX_DELAY)) {
            shared_imu.x = raw_x / SENSITIVITY_2G;
            shared_imu.y = raw_y / SENSITIVITY_2G;
            shared_imu.z = raw_z / SENSITIVITY_2G;
            xSemaphoreGive(xImuMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_console(void *pv) {
    while(1) {
        imu_data_t local_imu;
        
        if (xSemaphoreTake(xImuMutex, portMAX_DELAY)) {
            local_imu = shared_imu;
            xSemaphoreGive(xImuMutex);
        }

        int voltage_mv = (shared_pot_raw * 3300) / 4095;

        // Formatação solicitada pelo roteiro de testes
        printf("\033[H\033[J"); 
        printf("=====================================================\n");
        printf("STATUS: [%s] | POT: %d (%d mV) | LED: %d%%\n", 
               is_hold ? "HOLD" : "LIVE", 
               shared_pot_raw, 
               voltage_mv, 
               shared_led_pct);
        printf("IMU ACCEL (g): X: %.2f | Y: %.2f | Z: %.2f\n", 
               local_imu.x, local_imu.y, local_imu.z);
        printf("=====================================================\n");
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    init_hw();
    
    xPotQueue = xQueueCreate(5, sizeof(uint32_t));
    xBtnSemaphore = xSemaphoreCreateBinary();
    xImuMutex = xSemaphoreCreateMutex();

    xTaskCreate(task_button, "BTN", 2048, NULL, 5, NULL);
    xTaskCreate(task_led_control, "LED", 2048, NULL, 4, NULL);
    xTaskCreate(task_potentiometer, "POT", 2048, NULL, 3, NULL);
    xTaskCreate(task_imu, "IMU", 2048, NULL, 3, NULL);
    xTaskCreate(task_console, "CONS", 4096, NULL, 1, NULL);
}
