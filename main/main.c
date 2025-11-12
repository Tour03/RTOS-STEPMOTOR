#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define IN1 15
#define IN2 16
#define IN3 17
#define IN4 18
#define BTN_FWD 4
#define BTN_BWD 5

static const char *TAG = "STEPPER_RTOS";

// Half-step 
int sequence[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

int current_dir = 0; // 1 = forward, -1 = backward, 0 = stop

void step_motor(int dir, int delay_us) {
    if (dir == 0) return;

    if (dir > 0) {
        for (int step = 0; step < 8; step++) {
            gpio_set_level(IN1, sequence[step][0]);
            gpio_set_level(IN2, sequence[step][1]);
            gpio_set_level(IN3, sequence[step][2]);
            gpio_set_level(IN4, sequence[step][3]);
            vTaskDelay(pdMS_TO_TICKS(delay_us / 1000));
        }
    } else {
        for (int step = 7; step >= 0; step--) {
            gpio_set_level(IN1, sequence[step][0]);
            gpio_set_level(IN2, sequence[step][1]);
            gpio_set_level(IN3, sequence[step][2]);
            gpio_set_level(IN4, sequence[step][3]);
            vTaskDelay(pdMS_TO_TICKS(delay_us / 1000));
        }
    }
}

void release_motor() {
    gpio_set_level(IN1, 0);
    gpio_set_level(IN2, 0);
    gpio_set_level(IN3, 0);
    gpio_set_level(IN4, 0);
}

//  Task 1: อ่านปุ่ม (Forward / Backward)
void button_task(void *pvParameters) {
    while (1) {
        int fwd = gpio_get_level(BTN_FWD);
        int bwd = gpio_get_level(BTN_BWD);

        if (fwd == 0) {
            current_dir = 1;
        } else if (bwd == 0) {
            current_dir = -1;
        } else {
            current_dir = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // อ่านปุ่มทุก 50ms
    }
}

//  Task 2: ควบคุมมอเตอร์
void motor_task(void *pvParameters) {
    const int fast_speed_us = 1000;   // ความเร็วสำหรับ forward
    const int slow_speed_us = 500;  // ความเร็วสำหรับ backward

    while (1) {
        if (current_dir == 1) {
            step_motor(1, fast_speed_us);
        } else if (current_dir == -1) {
            step_motor(-1, slow_speed_us);
        } else {
            release_motor();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void app_main(void) {
    // ตั้งค่าขา output สำหรับมอเตอร์
    gpio_set_direction(IN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN3, GPIO_MODE_OUTPUT);
    gpio_set_direction(IN4, GPIO_MODE_OUTPUT);

    // ตั้งค่าปุ่ม input พร้อม pull-up
    gpio_set_direction(BTN_FWD, GPIO_MODE_INPUT);
    gpio_set_direction(BTN_BWD, GPIO_MODE_INPUT);
    gpio_pullup_en(BTN_FWD);
    gpio_pullup_en(BTN_BWD);
    gpio_pulldown_dis(BTN_FWD);
    gpio_pulldown_dis(BTN_BWD);

    ESP_LOGI(TAG, "Stepper RTOS control started");

    // สร้าง task แยก
    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
    xTaskCreate(motor_task, "motor_task", 4096, NULL, 5, NULL);
}
