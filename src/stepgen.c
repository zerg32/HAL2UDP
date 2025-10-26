/*
 * Implementation of step generation, timer scheduler ISR and helper math.
 * This was moved from the header into a single C file so symbols are
 * defined once and can be IRAM attributed where needed.
 */

#include "stepgen.h"
#include "globals.h"

#ifdef USE_I2S_OUT
#include "i2s_out.h"
#endif

// Single-timer scheduler ISR to support up to 5 axes

void IRAM_ATTR timer_sched_isr(void* arg)
{
    // previous alarm value (time delta)
    static uint32_t last_alarm = 10000UL;
    uint32_t dt = last_alarm;

    // per-axis static state
    static int _step_state[5] = {0,0,0,0,0};
    static int _local_dir[5] = {0,0,0,0,0};

    // remaining time until event for each axis
    static uint32_t remaining[5] = {10000UL,10000UL,10000UL,10000UL,10000UL};

    // Subtract dt from remaining timers and process events
    for (int i = 0; i < 5; ++i) {
        if (remaining[i] > dt) {
            remaining[i] -= dt;
            continue;
        }

        // remaining[i] <= dt -> event for axis i
        remaining[i] = 0;

        if (_step_state[i] == 0) {
            uint32_t t = T_half[i];
            if (t) {
                // start pulse (set step high)
                switch(i) {
                    case 0: STEP_0_H; break;
                    case 1: STEP_1_H; break;
                    case 2: STEP_2_H; break;
                    case 3: STEP_3_H; break;
                    case 4: STEP_4_H; break;
                }
                remaining[i] = t;
                if (_local_dir[i] == 0) --fb.pos[i]; else ++fb.pos[i];
                _step_state[i] = 1;
                math[i] = 1;
            } else {
                // no stepping needed; handle dir change or idle listen
                if (dirChange[i]) {
                    // set dir pin according to local dir
                    if (_local_dir[i] == 0) {
                        switch(i) { case 0: DIR_0_H; break; case 1: DIR_1_H; break; case 2: DIR_2_H; break; case 3: DIR_3_H; break; case 4: DIR_4_H; break; }
                    } else {
                        switch(i) { case 0: DIR_0_L; break; case 1: DIR_1_L; break; case 2: DIR_2_L; break; case 3: DIR_3_L; break; case 4: DIR_4_L; break; }
                    }
                    remaining[i] = dirSetup[i];
                    _local_dir[i] = dir[i] ^= 1; // toggle and store to global dir[]
                    dirChange[i] = 0;
                    math[i] = 1;
                } else {
                    remaining[i] = 10000UL; // idle listen
                    math[i] = 1;
                }
            }
        } else {
            // middle of period: set step low
            switch(i) {
                case 0: STEP_0_L; break;
                case 1: STEP_1_L; break;
                case 2: STEP_2_L; break;
                case 3: STEP_3_L; break;
                case 4: STEP_4_L; break;
            }
            uint32_t t = T_half[i];
            remaining[i] = t;
            _step_state[i] = 0;
        }
    }

    // compute next alarm = min(remaining[])
    uint32_t next_alarm = 0xFFFFFFFFU;
    for (int i = 0; i < 5; ++i) {
        if (remaining[i] && remaining[i] < next_alarm) next_alarm = remaining[i];
    }
    if (next_alarm == 0xFFFFFFFFU) next_alarm = 10000UL;

    last_alarm = next_alarm;
    timer_0_set_alarm_value(next_alarm);

    timer_0_clear_interrupt;
    timer_0_enable_alarm;
}

void timer__init(timer_group_t group, timer_idx_t idx)
{

    // Select and initialize basic parameters of the timer
    timer_config_t timer_config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .clk_src = TIMER_SRC_CLK_APB,
        .divider = 2UL,
    };

    timer_init(group, idx, &timer_config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, idx, 0ULL);

    /* Configure the alarm value and the interrupt on alarm. */
    // Set an initial alarm (microsecond-like units used by T_half)
    timer_set_alarm_value(group, idx, 10000ULL);
    timer_enable_intr(group, idx);
    if (group == TIMER_GROUP_0) {
        if (idx == TIMER_0) {
            // Register scheduler ISR only on TIMER_GROUP_0 / TIMER_0.
            // The scheduler manipulates TIMERG0 registers directly, so keep ISR bound to that timer.
            timer_isr_register(group, idx, timer_sched_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);
        }
    } else if (group == TIMER_GROUP_1) {
        // Do not register the scheduler ISR on TIMER_GROUP_1 to avoid cross-group register writes.
        // Other timers can be initialized but will not have the scheduler ISR attached.
    }

    timer_start(group, idx);
}

float fastInvSqrt(const float x)
{
    const float xhalf = x * 0.5f;
    union {
        float x;
        uint32_t i;
    } u = { .x = x };
    u.i = 0x5f3759df - (u.i >> 1);
    return u.x * (1.5f - xhalf * u.x * u.x);
}

void deceleration(const int i)
{
    if (accelStep[i] != 0) {
        if (--accelStep[i] != 0)
            T_half[i] = fastInvSqrt(accel_x2[i] * (float)accelStep[i]) * 20000000.0f;
        else
            T_half[i] = 0;
    }
}

void acceleration(const int i)
{
    if (cmd.control & CTRL_ENABLE) {
        ++accelStep[i];
        T_half[i] = fastInvSqrt(accel_x2[i] * (float)accelStep[i]) * 20000000.0f;
    } else
        deceleration(i);
}

void stepgen_task(void* arg)
{
#ifndef USE_I2S_OUT
    // Direct GPIO mode - initialize step/dir pins
    gpio_reset_pin((gpio_num_t)STEP_0_PIN);
    gpio_set_direction((gpio_num_t)STEP_0_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)DIR_0_PIN);
    gpio_set_direction((gpio_num_t)DIR_0_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)STEP_1_PIN);
    gpio_set_direction((gpio_num_t)STEP_1_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)DIR_1_PIN);
    gpio_set_direction((gpio_num_t)DIR_1_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)STEP_2_PIN);
    gpio_set_direction((gpio_num_t)STEP_2_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin((gpio_num_t)DIR_2_PIN);
    gpio_set_direction((gpio_num_t)DIR_2_PIN, GPIO_MODE_OUTPUT);
#endif
    // Note: In I2S mode, pins are initialized in i2s_out_init()

    timer__init(TIMER_GROUP_0, TIMER_0);
    timer__init(TIMER_GROUP_0, TIMER_1);
    timer__init(TIMER_GROUP_1, TIMER_0);

    for (;;) {
        for (int i = 0; i < 5; ++i) {
            if (math[i]) {
                if (accelStep[i]) {
                    if (dir[i] == cmd_dir[i]) {
                        if (T_half[i] > cmd_T_half[i])
                            acceleration(i);
                        else if (T_half[i] < cmd_T_half[i])
                            deceleration(i);
                    } else
                        deceleration(i);
                } else {
                    int pos_error = cmd.pos[i] - fb.pos[i];
                    if (pos_error < 0) {
                        if (dir[i] == 0)
                            acceleration(i);
                        else
                            dirChange[i] = 1;
                    } else if (pos_error > 0) {
                        if (dir[i] == 0)
                            dirChange[i] = 1;
                        else
                            acceleration(i);
                    }
                }
                math[i] = 0;
            }
        }
    }
}
