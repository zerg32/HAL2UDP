#pragma once

#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "hardware.h"

#define CTRL_DIRSETUP 0b00000001
#define CTRL_ACCEL    0b00000010
#define CTRL_PWMFREQ  0b00000100
#define CTRL_READY    0b01000000
#define CTRL_ENABLE   0b10000000

#define IO_00 0b00000001
#define IO_01 0b00000010
#define IO_02 0b00000100
#define IO_03 0b00001000
#define IO_04 0b00010000
#define IO_05 0b00100000
#define IO_06 0b01000000
#define IO_07 0b10000000

extern const uint8_t out_pins[6];

#pragma pack(push, 1)

typedef struct {
    union {
        int32_t pos[5];
        int32_t dirSetup[5];
        int32_t accel[5];
    };
    float vel[5];
    uint8_t control;
    uint8_t io;
    uint16_t pwm[6];
} cmd_t;

typedef struct {
    int32_t pos[5];
    float vel[5];
    uint8_t control;
    uint8_t io;
} fb_t;

extern cmd_t cmd;
extern volatile fb_t fb;

#pragma pack(pop)

extern volatile uint32_t dirSetup[5];
extern volatile float accel_x2[5];

extern volatile uint32_t cmd_T_half[5];
extern volatile int cmd_dir[5];

extern volatile uint32_t T_half[5];
extern volatile int dir[5];
extern volatile int dirChange[5];
extern volatile int math[5];

extern volatile uint32_t watchdog;

extern int pwm_enable[6];

extern uint32_t accelStep[5];

extern TaskHandle_t comm_task_handle;

extern SemaphoreHandle_t startMutex;

