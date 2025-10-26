// I2S Output Driver for HAL2UDP
// Based on FluidNC's I2S implementation
// Copyright (c) 2020 - Michiyasu Odaki (FluidNC)
// Adapted for HAL2UDP by jzolee
// Use of this source code is governed by a GPLv3 license

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <esp_attr.h>

// Number of I2S output pins available (32 bits in shift register)
#define I2S_OUT_NUM_BITS 32

// I2S configuration structure
typedef struct {
    /*
        I2S bitstream (32-bits): Transfers from MSB(bit31) to LSB(bit0) in sequence
        ------------------time line------------------------>
             Left Channel                    Right Channel
        ws   ________________________________~~~~...
        bck  _~_~_~_~_~_~_~_~_~_~_~_~_~_~_~_~_~_~...
        data vutsrqponmlkjihgfedcba9876543210
             XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
                                             ^
                                Latches the X bits when ws is switched to High
        
        If using 74HC595 or similar shift registers:
        - ws_pin connects to RCLK (latch/register clock)
        - bck_pin connects to SRCLK (shift register clock)  
        - data_pin connects to SER (serial data input)
    */
    uint8_t  ws_pin;        // Word select / Latch pin
    uint8_t  bck_pin;       // Bit clock / Shift clock pin
    uint8_t  data_pin;      // Serial data pin
    uint32_t init_val;      // Initial value to output
    uint32_t pulse_us;      // Pulse duration in microseconds (1, 2, or 4)
} i2s_out_init_t;

/*
  Initialize I2S output system
  Returns: 0 on success, -1 if already initialized
*/
int i2s_out_init(i2s_out_init_t* init_param);

/*
  Read the current state of an I2S output pin
  pin: I2S pin number (0..31)
  Returns: 0 or 1
*/
uint8_t IRAM_ATTR i2s_out_read(uint8_t pin);

/*
  Set a bit in the internal pin state (queued for output via I2S FIFO)
  pin: I2S pin number (0..31)
  val: bit value (0 or non-zero)
*/
void IRAM_ATTR i2s_out_write(uint8_t pin, uint8_t val);

/*
  Delay until I2S shift register has updated
  Use after writes when immediate hardware update is required
*/
void i2s_out_delay(void);

/*
  Start I2S transmission
*/
void i2s_out_start(void);

/*
  Stop I2S transmission
*/
void i2s_out_stop(void);

#ifdef __cplusplus
}
#endif
