// I2S Output Driver for HAL2UDP
// Based on FluidNC's I2S implementation
// Copyright (c) 2020 - Michiyasu Odaki (FluidNC)
// Copyright (c) 2024 - Mitch Bradley (FluidNC)
// Adapted for HAL2UDP by jzolee
// Use of this source code is governed by a GPLv3 license

#include "i2s_out.h"
#include "hardware.h"

#include <string.h>
#include <esp_attr.h>
#include <driver/periph_ctrl.h>
#include <soc/i2s_struct.h>
#include <soc/gpio_periph.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <driver/i2s_std.h>

// Minimal GPIO-backed fallback implementation for I2S-like shift register support.
// This avoids dependency on low-level i2s_ll APIs that may not be available in
// all ESP-IDF versions. It provides a simple in-memory port state and basic
// GPIO initialization so the project links and can be tested. It is not as
// efficient as a proper I2S FIFO implementation and may not be suitable for
// high-rate step signals; it's a safe fallback for builds.

#ifdef USE_I2S_OUT

// Global state
static volatile uint32_t i2s_out_port_data = 0;
static int i2s_out_initialized = 0;
static uint8_t i2s_out_ws_pin = 255;
static uint8_t i2s_out_bck_pin = 255;
static uint8_t i2s_out_data_pin = 255;
static uint32_t i2s_frame_us = 2; // Default 2us pulse width

// Modern I2S STD channel handle (TX)
static i2s_chan_handle_t i2s_tx_chan = NULL;

// (No DMA task — ISR writes directly to I2S FIFO)

// Simple GPIO helpers
static inline void i2s_gpio_setup_pin(uint8_t pin)
{
    if (pin > 0 && pin <= 40) {
        gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        gpio_set_level(pin, 0);
    }
}

// Note: ISR-only I2S FIFO writes are used. GPIO bit-bang fallback removed
// to keep the implementation minimal and focused on the high-performance path.

// No DMA task: i2s_out_write writes directly to I2S FIFO from ISR for minimal latency.

void i2s_out_stop(void)
{
    if (!i2s_out_initialized) return;
    // Flush current state into FIFO (byte-swapped for MSB-first) then disable channel
    if (i2s_tx_chan) {
        uint32_t v = __builtin_bswap32(i2s_out_port_data);
        I2S0.fifo_wr = v;
        i2s_channel_disable(i2s_tx_chan);
    }
}

void i2s_out_start(void)
{
    if (!i2s_out_initialized) return;
    // Enable the TX channel and pre-fill FIFO with current state
    if (i2s_tx_chan) {
        i2s_channel_enable(i2s_tx_chan);
        uint32_t v = __builtin_bswap32(i2s_out_port_data);
        I2S0.fifo_wr = v;
    }
}

void IRAM_ATTR i2s_out_write(uint8_t pin, uint8_t val)
{
    if (pin >= I2S_OUT_NUM_BITS) return;
    uint32_t mask = 1UL << pin;
    if (val)
        i2s_out_port_data |= mask;
    else
        i2s_out_port_data &= ~mask;
    // Fast path: write directly to I2S FIFO from ISR for minimal latency.
    // Use byte-swap so MSB is shifted out first (matches shift-register layout).
    if (i2s_out_initialized) {
        uint32_t v = __builtin_bswap32(i2s_out_port_data);
        I2S0.fifo_wr = v;
    }
}

uint8_t i2s_out_read(uint8_t pin)
{
    if (pin >= I2S_OUT_NUM_BITS) return 0;
    return !!(i2s_out_port_data & (1UL << pin));
}

void i2s_out_delay(void)
{
    // simple delay based on frame width
    ets_delay_us(i2s_frame_us);
}

int i2s_out_init(i2s_out_init_t* init_param)
{
    if (i2s_out_initialized) return -1;
    if (!init_param) return -1;

    if (init_param->pulse_us != 1 && init_param->pulse_us != 2 && init_param->pulse_us != 4) {
        init_param->pulse_us = 2;
    }
    i2s_frame_us = init_param->pulse_us;
    i2s_out_port_data = init_param->init_val;

    i2s_out_ws_pin = init_param->ws_pin;
    i2s_out_bck_pin = init_param->bck_pin;
    i2s_out_data_pin = init_param->data_pin;

    // configure GPIO pins
    i2s_gpio_setup_pin(i2s_out_ws_pin);
    i2s_gpio_setup_pin(i2s_out_bck_pin);
    i2s_gpio_setup_pin(i2s_out_data_pin);

    i2s_out_initialized = 1;

    // initial output
    // Use modern I2S STD channel API to configure TX channel; ISR will still
    // write directly to the I2S FIFO for lowest latency.
    {
        // Allocate a TX channel (auto controller selection)
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
        if (i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL) == ESP_OK) {
            // Configure standard mode (clock/slot/gpio)
            int sample_rate = (int)(1000000U / i2s_frame_us);
            i2s_std_config_t std_cfg = {
                .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
                .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
                .gpio_cfg = {
                    .mclk = I2S_GPIO_UNUSED,
                    .bclk = i2s_out_bck_pin,
                    .ws   = i2s_out_ws_pin,
                    .dout = i2s_out_data_pin,
                    .din  = I2S_GPIO_UNUSED,
                    .invert_flags = {
                        .mclk_inv = false,
                        .bclk_inv = false,
                        .ws_inv   = false,
                    },
                },
            };
            i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg);
        }
    }

    i2s_out_start();
    return 0;
}

#endif /* USE_I2S_OUT */
