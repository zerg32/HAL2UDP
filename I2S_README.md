# I2S Shift Register Support for HAL2UDP

This document describes the optional I2S shift register mode for driving step/dir signals through external shift registers like the 74HC595.

## Overview

HAL2UDP can drive step and direction signals in two modes:

1. **Direct GPIO Mode** (default): Step/dir signals go directly to ESP32 GPIO pins
2. **I2S Shift Register Mode**: Step/dir signals are sent to shift registers (74HC595, etc.) via the I2S peripheral

The I2S mode allows you to:
- Use fewer GPIO pins (only 3 pins for unlimited outputs via shift register chains)
- Extend the number of step/dir outputs
- Synchronize all outputs with precise timing via hardware DMA

## Hardware Setup

### Required Components
- ESP32 with W5500 Ethernet module
- 74HC595 shift register(s) or compatible (74HC164, CD4094, etc.)
- Pull-down resistors on shift register outputs (optional but recommended)

### Wiring Example (74HC595)

```
ESP32 GPIO 26 (I2S_WS)   -> 74HC595 Pin 12 (RCLK / Storage Register Clock)
ESP32 GPIO 27 (I2S_BCK)  -> 74HC595 Pin 11 (SRCLK / Shift Register Clock)
ESP32 GPIO 32 (I2S_DATA) -> 74HC595 Pin 14 (SER / Serial Data Input)

74HC595 Pin 10 (SRCLR)   -> VCC (disable clear)
74HC595 Pin 13 (OE)      -> GND (enable output)
74HC595 Pin 16 (VCC)     -> 3.3V or 5V
74HC595 Pin 8 (GND)      -> GND

Output mapping (default):
74HC595 QA (Pin 15) -> STEP 0
74HC595 QB (Pin 1)  -> DIR 0
74HC595 QC (Pin 2)  -> STEP 1
74HC595 QD (Pin 3)  -> DIR 1
74HC595 QE (Pin 4)  -> STEP 2
74HC595 QF (Pin 5)  -> DIR 2
...
```

### Chaining Multiple Shift Registers

To drive more outputs, chain 74HC595s:

```
First IC Pin 9 (Q7') -> Second IC Pin 14 (SER)
```

This gives you 32 outputs total (4 ICs x 8 bits).

## Software Configuration

### Enable I2S Mode

1. Open `src/hardware.h`
2. Uncomment the line: `#define USE_I2S_OUT`
3. Configure pins and bit mappings:

```c
#define USE_I2S_OUT

#ifdef USE_I2S_OUT
    // I2S pins
    #define I2S_WS_PIN      26  // RCLK (Register/Latch Clock)
    #define I2S_BCK_PIN     27  // SRCLK (Shift Register Clock)
    #define I2S_DATA_PIN    32  // SER (Serial Data)
    
    // Pulse width: 1, 2, or 4 microseconds
    #define I2S_PULSE_US    2
    
    // Bit mappings (shift register bit positions 0-31)
    #define I2S_STEP_0_BIT  0
    #define I2S_DIR_0_BIT   1
    #define I2S_STEP_1_BIT  2
    #define I2S_DIR_1_BIT   3
    #define I2S_STEP_2_BIT  4
    #define I2S_DIR_2_BIT   5
#endif
```

4. Build and flash:
```bash
pio run -t upload
```

## How It Works

The I2S peripheral on the ESP32 is normally used for audio, but it can be repurposed to drive shift registers:

- **I2S_WS (Word Select)**: Acts as RCLK - latches data from shift to storage register
- **I2S_BCK (Bit Clock)**: Acts as SRCLK - clocks data into shift register
- **I2S_DATA**: Serial data output

The ESP32's I2S DMA continuously streams a 32-bit word to the shift registers, updating all outputs simultaneously. This provides:
- Hardware-synchronized outputs
- Low CPU overhead (DMA handles data transfer)
- Precise timing control (1, 2, or 4 μs pulse widths)

## Implementation Details

### Based on FluidNC

This implementation is adapted from [FluidNC](https://github.com/bdring/FluidNC), a popular CNC controller firmware. The core I2S driver files (`src/i2s_out.c` and `src/i2s_out.h`) are based on FluidNC's implementation with simplifications for HAL2UDP's use case.

### Conditional Compilation

The code uses `#ifdef USE_I2S_OUT` to switch between modes:

- In `src/stepgen.h`: Step/dir macros call either GPIO register writes or `i2s_out_write()`
- In `src/main.c`: I2S initialization only runs when enabled
- GPIO pins for step/dir are only configured in direct GPIO mode

### Timing Considerations

- Default pulse width: 2 μs (suitable for most stepper drivers)
- For faster steppers: Use 1 μs
- For slower steppers or optocouplers: Use 4 μs
- Maximum step frequency depends on pulse width and LinuxCNC servo thread

## Troubleshooting

### No outputs / all outputs stuck
- Check power to shift register (VCC, GND)
- Verify OE (Output Enable) is tied to GND
- Check wiring of I2S pins

### Incorrect outputs / garbage data
- Verify bit mapping in `hardware.h` matches your wiring
- Check that SRCLR (clear) is tied high
- Ensure proper ground connection between ESP32 and shift register

### Step pulses too short/long
- Adjust `I2S_PULSE_US` in `hardware.h` (1, 2, or 4)
- Check stepper driver minimum pulse width requirements

## GPIO Pin Notes

When I2S mode is enabled, the default step/dir GPIO pins (12, 13, 16, 17, 21, 22) are **not used**. You can:
- Leave them unconnected
- Repurpose them for additional inputs/outputs
- Use them for other features

The I2S pins (26, 27, 32) **must not** be used for inputs when I2S is enabled, as they become I2S peripheral outputs.

## Performance

I2S mode provides:
- Step rates up to 100 kHz (same as direct GPIO mode)
- Lower interrupt overhead (updates via DMA)
- Synchronized multi-axis updates
- Expandable to 32+ outputs with shift register chains

## License

I2S driver based on FluidNC implementation:
- Copyright (c) 2020 - Michiyasu Odaki
- Copyright (c) 2024 - Mitch Bradley
- GPLv3 License
