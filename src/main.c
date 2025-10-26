#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "globals.h"
#include "io.h"
#include "comm.h"
#include "stepgen.h"

#ifdef USE_I2S_OUT
#include "i2s_out.h"
#include "esp_log.h"
static const char* TAG = "HAL2UDP";
#endif

void app_main(void)
{
    startMutex = xSemaphoreCreateMutex();
    io_init();
    
#ifdef USE_I2S_OUT
    // Initialize I2S for shift register control
    i2s_out_init_t i2s_config = {
        .ws_pin = I2S_WS_PIN,
        .bck_pin = I2S_BCK_PIN,
        .data_pin = I2S_DATA_PIN,
        .init_val = 0,
        .pulse_us = I2S_PULSE_US
    };
    
    int ret = i2s_out_init(&i2s_config);
    if (ret == 0) {
        ESP_LOGI(TAG, "I2S output initialized: WS=%d BCK=%d DATA=%d pulse=%dus", 
                 I2S_WS_PIN, I2S_BCK_PIN, I2S_DATA_PIN, I2S_PULSE_US);
    } else {
        ESP_LOGE(TAG, "Failed to initialize I2S output!");
    }
#endif

    xTaskCreatePinnedToCore(comm_task, "comm_task", 8192, NULL, 2, &comm_task_handle, 0);
    xTaskCreatePinnedToCore(watchdog_task, "watchdog_task", 2048, NULL, 1, NULL, 0);
    xSemaphoreTake(startMutex, portMAX_DELAY);
    xTaskCreatePinnedToCore(stepgen_task, "stepgen_task", 4096, NULL, 1, NULL, 1);
}
