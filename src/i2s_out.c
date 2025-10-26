// I2S Output Driver for HAL2UDP
// Based on FluidNC's I2S implementation
// Copyright (c) 2020 - Michiyasu Odaki (FluidNC)
// Copyright (c) 2024 - Mitch Bradley (FluidNC)
// Adapted for HAL2UDP by jzolee
// Use of this source code is governed by a GPLv3 license

#include "i2s_out.h"

#include <string.h>
#include <esp_attr.h>
#include <driver/periph_ctrl.h>
#include <soc/i2s_struct.h>
#include <soc/gpio_periph.h>
#include <hal/i2s_ll.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>

// Global state
static volatile uint32_t i2s_out_port_data = 0;
static int i2s_out_initialized = 0;
static uint8_t i2s_out_ws_pin = 255;
static uint8_t i2s_out_bck_pin = 255;
static uint8_t i2s_out_data_pin = 255;
static uint32_t i2s_frame_us = 2; // Default 2us pulse width

// Helper functions for GPIO routing
static void gpio_route(uint8_t gpio_num, uint8_t signal_idx) {
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
    gpio_matrix_out(gpio_num, signal_idx, false, false);
}

static void gpio_write(uint8_t gpio_num, uint8_t value) {
    if (value) {
        gpio_set_level(gpio_num, 1);
    } else {
        gpio_set_level(gpio_num, 0);
    }
}

// I2S low-level control
static inline void i2s_out_reset_tx_rx(void) {
    i2s_ll_tx_reset(&I2S0);
    i2s_ll_rx_reset(&I2S0);
}

static inline void i2s_out_reset_fifo(void) {
    i2s_ll_tx_reset_fifo(&I2S0);
    i2s_ll_rx_reset_fifo(&I2S0);
}

static int i2s_out_gpio_attach(uint8_t ws, uint8_t bck, uint8_t data) {
    // Route I2S signals to GPIO pins
    gpio_route(data, I2S0O_DATA_OUT23_IDX);
    gpio_route(bck, I2S0O_BCK_OUT_IDX);
    gpio_route(ws, I2S0O_WS_OUT_IDX);
    return 0;
}

static int i2s_out_gpio_detach(uint8_t ws, uint8_t bck, uint8_t data) {
    const int I2S_OUT_DETACH_PORT_IDX = 0x100;
    gpio_route(ws, I2S_OUT_DETACH_PORT_IDX);
    gpio_route(bck, I2S_OUT_DETACH_PORT_IDX);
    gpio_route(data, I2S_OUT_DETACH_PORT_IDX);
    return 0;
}

static int i2s_out_gpio_shiftout(uint32_t port_data) {
    // Manually shift out data when I2S is stopped
    gpio_write(i2s_out_ws_pin, 0);
    for (int i = 0; i < I2S_OUT_NUM_BITS; i++) {
        gpio_write(i2s_out_data_pin, !!(port_data & (1 << (I2S_OUT_NUM_BITS - 1 - i))));
        gpio_write(i2s_out_bck_pin, 1);
        gpio_write(i2s_out_bck_pin, 0);
    }
    gpio_write(i2s_out_ws_pin, 1); // Latch
    return 0;
}

void i2s_out_stop(void) {
    if (!i2s_out_initialized) {
        return;
    }

    // Stop TX module
    i2s_ll_tx_stop(&I2S0);

    // Force WS to LOW before detach
    gpio_write(i2s_out_ws_pin, 0);

    // Detach GPIO pins from I2S
    i2s_out_gpio_detach(i2s_out_ws_pin, i2s_out_bck_pin, i2s_out_data_pin);

    // Force BCK to LOW
    gpio_write(i2s_out_bck_pin, 0);

    // Transmit current data to shift register
    uint32_t port_data = i2s_out_port_data;
    i2s_out_gpio_shiftout(port_data);
}

void i2s_out_start(void) {
    if (!i2s_out_initialized) {
        return;
    }

    // Transmit initial data to shift register
    uint32_t port_data = i2s_out_port_data;
    i2s_out_gpio_shiftout(port_data);

    // Attach I2S to GPIO pins
    i2s_out_gpio_attach(i2s_out_ws_pin, i2s_out_bck_pin, i2s_out_data_pin);

    // Reset TX/RX module and FIFO
    i2s_out_reset_tx_rx();
    i2s_out_reset_fifo();

    // Start TX
    i2s_ll_tx_stop_on_fifo_empty(&I2S0, true);
    i2s_ll_tx_start(&I2S0);

    // Wait for first FIFO data to prevent unintentional 0 data
    ets_delay_us(20);
    i2s_ll_tx_stop_on_fifo_empty(&I2S0, false);
}

void IRAM_ATTR i2s_out_write(uint8_t pin, uint8_t val) {
    if (pin >= I2S_OUT_NUM_BITS) {
        return;
    }

    uint32_t bit = 1 << pin;
    if (val) {
        i2s_out_port_data |= bit;
    } else {
        i2s_out_port_data &= ~bit;
    }

    // Write directly to FIFO for immediate update
    if (i2s_out_initialized) {
        I2S0.fifo_wr = i2s_out_port_data;
    }
}

uint8_t i2s_out_read(uint8_t pin) {
    if (pin >= I2S_OUT_NUM_BITS) {
        return 0;
    }
    uint32_t port_data = i2s_out_port_data;
    return !!(port_data & (1 << pin));
}

void i2s_out_delay(void) {
    // Wait for shift register to update
    // FIFO_LENGTH is typically 64 for ESP32
    const int FIFO_LENGTH = 64;
    uint32_t wait_counts = FIFO_LENGTH;
    ets_delay_us(i2s_frame_us * wait_counts);
}

int i2s_out_init(i2s_out_init_t* init_param) {
    if (i2s_out_initialized) {
        return -1; // Already initialized
    }

    if (init_param == NULL) {
        return -1;
    }

    // Validate pulse width parameter
    if (init_param->pulse_us != 1 && init_param->pulse_us != 2 && init_param->pulse_us != 4) {
        init_param->pulse_us = 2; // Default to 2us
    }
    i2s_frame_us = init_param->pulse_us;

    i2s_out_port_data = init_param->init_val;

    // Enable I2S peripheral
    periph_module_reset(PERIPH_I2S0_MODULE);
    periph_module_enable(PERIPH_I2S0_MODULE);

    // Initialize GPIO pins
    gpio_route(init_param->ws_pin, I2S0O_WS_OUT_IDX);
    gpio_route(init_param->bck_pin, I2S0O_BCK_OUT_IDX);
    gpio_route(init_param->data_pin, I2S0O_DATA_OUT23_IDX);

    /*
     * I2S clock configuration:
     * 
     * fpll = PLL_D2_CLK = 160MHz (clka_en = 0)
     * fi2s = fpll / (N + b/a) where N = clkm_div_num
     * fbclk = fi2s / M where M = tx_bck_div_num
     * fwclk = fbclk / 32
     * 
     * For different pulse widths:
     * - 1us: N=2, b/a=2/1 (2.5), M=2 -> fwclk=1MHz
     * - 2us: N=5, b/a=0, M=2 -> fwclk=500kHz
     * - 4us: N=10, b/a=0, M=2 -> fwclk=250kHz
     */

    // Stop I2S
    i2s_ll_tx_stop_link(&I2S0);
    i2s_ll_tx_stop(&I2S0);

    // Reset FIFO
    i2s_out_reset_fifo();

    // Configure I2S mode
    i2s_ll_enable_lcd(&I2S0, false);
    i2s_ll_enable_camera(&I2S0, false);
#ifdef SOC_I2S_SUPPORTS_PDM_TX
    i2s_ll_tx_enable_pdm(&I2S0, false);
#endif

    i2s_ll_enable_dma(&I2S0, false);

    i2s_ll_tx_set_chan_mod(&I2S0, I2S_CHANNEL_FMT_RIGHT_LEFT);
    i2s_ll_tx_set_sample_bit(&I2S0, I2S_BITS_PER_SAMPLE_32BIT, I2S_BITS_PER_SAMPLE_16BIT);
    i2s_ll_tx_enable_mono_mode(&I2S0, false);

    i2s_ll_tx_stop(&I2S0);
    i2s_ll_rx_stop(&I2S0);

    i2s_ll_tx_enable_msb_right(&I2S0, true);
    i2s_ll_tx_enable_right_first(&I2S0, false);

    i2s_ll_tx_set_slave_mod(&I2S0, false); // Master mode
    i2s_ll_tx_force_enable_fifo_mod(&I2S0, true);

#ifdef SOC_I2S_SUPPORTS_PDM_TX
    i2s_ll_tx_enable_pdm(&I2S0, false);
#endif

    // I2S_COMM_FORMAT_I2S_LSB
    i2s_ll_tx_set_ws_width(&I2S0, 0);
    i2s_ll_tx_enable_msb_shift(&I2S0, false);

#ifdef CONFIG_IDF_TARGET_ESP32
    i2s_ll_tx_clk_set_src(&I2S0, I2S_CLK_D2CLK);
#endif

    // Set clock divider based on pulse width
    i2s_ll_mclk_div_t div;
    switch (i2s_frame_us) {
        case 1:
            div.mclk_div = 2;
            div.a = 32;
            div.b = 16; // 2 + 16/32 = 2.5
            break;
        case 2:
            div.mclk_div = 5;
            div.a = 0;
            div.b = 0;
            break;
        case 4:
        default:
            div.mclk_div = 10;
            div.a = 0;
            div.b = 0;
            break;
    }
    i2s_ll_tx_set_clk(&I2S0, &div);
    i2s_ll_tx_set_bck_div_num(&I2S0, 2);

    // Remember GPIO pin numbers
    i2s_out_ws_pin = init_param->ws_pin;
    i2s_out_bck_pin = init_param->bck_pin;
    i2s_out_data_pin = init_param->data_pin;
    i2s_out_initialized = 1;

    // Start I2S peripheral
    i2s_out_start();

    return 0;
}
