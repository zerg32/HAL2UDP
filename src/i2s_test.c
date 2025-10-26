/*
 * Minimal I2S hardware test task
 * Pulses motors 3 and 4 STEP lines so you can capture oscilloscope traces.
 * Enable by defining ENABLE_I2S_TEST in build flags or in src/hardware.h
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "stepgen.h"

#ifdef ENABLE_I2S_TEST

void i2s_test_task(void* arg)
{
    (void)arg;

    // Ensure I2S is started if available
#ifdef USE_I2S_OUT
    i2s_out_start();
#endif

    // Pulse parameters
    const TickType_t pulse_ms = 1;   // step high duration in ms (easy to see on scope)
    const TickType_t period_ms = 10; // period between pulses

    for (;;) {
        // Motor 3
        STEP_3_H;
        vTaskDelay(pdMS_TO_TICKS(pulse_ms));
        STEP_3_L;

        vTaskDelay(pdMS_TO_TICKS(period_ms - pulse_ms));

        // Motor 4 (phase-shifted)
        STEP_4_H;
        vTaskDelay(pdMS_TO_TICKS(pulse_ms));
        STEP_4_L;

        vTaskDelay(pdMS_TO_TICKS(period_ms - pulse_ms));
    }
}

#endif // ENABLE_I2S_TEST
