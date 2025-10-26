/*
 * Definitions for globals used across the firmware.
 * These were moved out of the header to avoid multiple definition
 * linker errors when many .c files include the header.
 */

#include "globals.h"

const uint8_t out_pins[6] = { OUT_00_PIN, OUT_01_PIN, OUT_02_PIN, OUT_03_PIN, OUT_04_PIN, OUT_05_PIN };

cmd_t cmd = { 0 };
volatile fb_t fb = { 0 };

volatile uint32_t dirSetup[5] = { 1000, 1000, 1000, 1000, 1000 }; // x 25 nanosec
volatile float accel_x2[5] = { 1000.0f, 1000.0f, 1000.0f, 1000.0f, 1000.0f }; // acceleration*2 step/sec2

volatile uint32_t cmd_T_half[5] = { 0 };
volatile int cmd_dir[5] = { 0 };

volatile uint32_t T_half[5] = { 0 };
volatile int dir[5] = { 0 };
volatile int dirChange[5] = { 0 };
volatile int math[5] = { 0 };

volatile uint32_t watchdog = 0;

int pwm_enable[6] = { 0 };

uint32_t accelStep[5] = { 0 };

TaskHandle_t comm_task_handle = NULL;

SemaphoreHandle_t startMutex = NULL;
