#pragma once

#include <stdint.h>

#include "esp_attr.h"
#include "driver/timer.h" //#include "driver/gptimer.h" //new driver for future use
#include "soc/timer_group_struct.h"
#include "driver/gpio.h"

#include "globals.h"
#include "hardware.h"

#ifdef USE_I2S_OUT
#include "i2s_out.h"
#endif

//#define timer_0_set_alarm_value(alarm_val) timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_0, alarm_val)
#define timer_0_set_alarm_value(alarm_val) TIMERG0.hw_timer[0].alarmlo.tx_alarm_lo = alarm_val
// #define timer_0_set_alarm_value(alarm_val) TIMERG0.hw_timer[0].alarm_low = alarm_val

//#define timer_0_clear_interrupt timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0)
#define timer_0_clear_interrupt TIMERG0.int_clr_timers.t0_int_clr = 1
// #define timer_0_clear_interrupt TIMERG0.int_clr_timers.t0 = 1

//#define timer_0_enable_alarm timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0)
#define timer_0_enable_alarm TIMERG0.hw_timer[0].config.tx_alarm_en = 1
//#define timer_0_enable_alarm TIMERG0.hw_timer[0].config.alarm_en = 1

//#define timer_1_set_alarm_value(alarm_val) timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_1, alarm_val)
#define timer_1_set_alarm_value(alarm_val) TIMERG0.hw_timer[1].alarmlo.tx_alarm_lo = alarm_val
//#define timer_1_set_alarm_value(alarm_val) TIMERG0.hw_timer[1].alarm_low = alarm_val

//#define timer_1_clear_interrupt timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_1)
#define timer_1_clear_interrupt TIMERG0.int_clr_timers.t1_int_clr = 1
//#define timer_1_clear_interrupt TIMERG0.int_clr_timers.t1 = 1

//#define timer_1_enable_alarm timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_1)
#define timer_1_enable_alarm TIMERG0.hw_timer[1].config.tx_alarm_en = 1
//#define timer_1_enable_alarm TIMERG0.hw_timer[1].config.alarm_en = 1

//#define timer_2_set_alarm_value(alarm_val) timer_group_set_alarm_value_in_isr(TIMER_GROUP_1, TIMER_0, alarm_val)
#define timer_2_set_alarm_value(alarm_val) TIMERG1.hw_timer[0].alarmlo.tx_alarm_lo = alarm_val
//#define timer_2_set_alarm_value(alarm_val) TIMERG1.hw_timer[0].alarm_low = alarm_val

//#define timer_2_clear_interrupt timer_group_clr_intr_status_in_isr(TIMER_GROUP_1, TIMER_0)
#define timer_2_clear_interrupt TIMERG1.int_clr_timers.t0_int_clr = 1
//#define timer_2_clear_interrupt TIMERG1.int_clr_timers.t0 = 1

//#define timer_2_enable_alarm timer_group_enable_alarm_in_isr(TIMER_GROUP_1, TIMER_0)
//#define timer_2_enable_alarm TIMERG1.hw_timer[0].config.tx_alarm_en = 1
//#define timer_2_enable_alarm TIMERG1.hw_timer[0].config.alarm_en = 1

/*==================================================================*/
/* Step/Dir pin control macros - support both GPIO and I2S modes   */
/*==================================================================*/

#ifdef USE_I2S_OUT
    // I2S mode - write to shift register via I2S FIFO
    #define STEP_0_H i2s_out_write(I2S_STEP_0_BIT, 1)
    #define STEP_0_L i2s_out_write(I2S_STEP_0_BIT, 0)
    #define DIR_0_H i2s_out_write(I2S_DIR_0_BIT, 1)
    #define DIR_0_L i2s_out_write(I2S_DIR_0_BIT, 0)

    #define STEP_1_H i2s_out_write(I2S_STEP_1_BIT, 1)
    #define STEP_1_L i2s_out_write(I2S_STEP_1_BIT, 0)
    #define DIR_1_H i2s_out_write(I2S_DIR_1_BIT, 1)
    #define DIR_1_L i2s_out_write(I2S_DIR_1_BIT, 0)

    #define STEP_2_H i2s_out_write(I2S_STEP_2_BIT, 1)
    #define STEP_2_L i2s_out_write(I2S_STEP_2_BIT, 0)
    #define DIR_2_H i2s_out_write(I2S_DIR_2_BIT, 1)
    #define DIR_2_L i2s_out_write(I2S_DIR_2_BIT, 0)
    
    #define STEP_3_H i2s_out_write(I2S_STEP_3_BIT, 1)
    #define STEP_3_L i2s_out_write(I2S_STEP_3_BIT, 0)
    #define DIR_3_H i2s_out_write(I2S_DIR_3_BIT, 1)
    #define DIR_3_L i2s_out_write(I2S_DIR_3_BIT, 0)

    #define STEP_4_H i2s_out_write(I2S_STEP_4_BIT, 1)
    #define STEP_4_L i2s_out_write(I2S_STEP_4_BIT, 0)
    #define DIR_4_H i2s_out_write(I2S_DIR_4_BIT, 1)
    #define DIR_4_L i2s_out_write(I2S_DIR_4_BIT, 0)
#else
    // Direct GPIO mode - use register writes (default)
    #define STEP_0_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT12)
    #define STEP_0_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT12)
    #define DIR_0_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT13)
    #define DIR_0_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT13)

    #define STEP_1_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT16)
    #define STEP_1_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT16)
    #define DIR_1_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT17)
    #define DIR_1_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT17)

    #define STEP_2_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT21)
    #define STEP_2_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT21)
    #define DIR_2_H REGISTER_WRITE(GPIO_OUT_W1TS_REG, BIT22)
    #define DIR_2_L REGISTER_WRITE(GPIO_OUT_W1TC_REG, BIT22)

    // For direct GPIO mode, motors 3 and 4 are not mapped to physical GPIOs by default.
    // Define no-op macros so firmware compiles; for full GPIO-based 5-axis support, map these to real pins.
    #define STEP_3_H ((void)0)
    #define STEP_3_L ((void)0)
    #define DIR_3_H  ((void)0)
    #define DIR_3_L  ((void)0)

    #define STEP_4_H ((void)0)
    #define STEP_4_L ((void)0)
    #define DIR_4_H  ((void)0)
    #define DIR_4_L  ((void)0)
#endif

/*==================================================================*/

// Prototypes for functions implemented in src/stepgen.c
void timer_sched_isr(void* arg);
void timer__init(timer_group_t group, timer_idx_t idx);
float fastInvSqrt(const float x);
void deceleration(const int i);
void acceleration(const int i);
void stepgen_task(void* arg);
