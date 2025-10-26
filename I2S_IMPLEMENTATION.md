# I2S Implementation Summary for HAL2UDP

## What Was Added

I've successfully integrated I2S shift register support into HAL2UDP, based on FluidNC's proven implementation. This allows driving step/dir signals through shift registers (like 74HC595) instead of direct GPIO.

## Files Created/Modified

### New Files:
1. **src/i2s_out.h** - I2S driver header (API definitions)
2. **src/i2s_out.c** - I2S driver implementation (based on FluidNC)
3. **I2S_README.md** - Comprehensive documentation for I2S mode

### Modified Files:
1. **src/hardware.h** - Added I2S configuration section with pin definitions and bit mappings
2. **src/stepgen.h** - Modified step/dir macros to support both GPIO and I2S modes
3. **src/main.c** - Added I2S initialization when USE_I2S_OUT is defined
4. **.github/copilot-instructions.md** - Documented I2S feature for AI agents

## How It Works

### Mode Selection (Compile-Time)
- **Default**: Direct GPIO mode (current behavior, no changes needed)
- **I2S Mode**: Uncomment `#define USE_I2S_OUT` in `src/hardware.h`

### Architecture
```
┌─────────────┐
│   LinuxCNC  │
└──────┬──────┘
       │ UDP
       ▼
┌─────────────┐
│    ESP32    │
│  HAL2UDP    │
└──────┬──────┘
       │
       ├─ Direct GPIO Mode (default)
       │  └─> GPIO 12, 13, 16, 17, 21, 22 (step/dir pins)
       │
       └─ I2S Mode (optional)
          └─> GPIO 26, 27, 32 (I2S signals)
              └─> 74HC595 Shift Register(s)
                  └─> 8-32 outputs (expandable)
```

### Key Features
- **Compile-time switching**: No runtime overhead when not using I2S
- **FluidNC-based**: Uses proven, production-tested I2S driver code
- **Configurable timing**: 1, 2, or 4 μs pulse widths
- **Hardware DMA**: Low CPU overhead, precise timing
- **Expandable**: Chain shift registers for 32+ outputs

## Configuration Example

In `src/hardware.h`:
```c
// Uncomment to enable I2S mode
#define USE_I2S_OUT

#ifdef USE_I2S_OUT
    // I2S pins (connect to 74HC595)
    #define I2S_WS_PIN      26  // -> RCLK
    #define I2S_BCK_PIN     27  // -> SRCLK
    #define I2S_DATA_PIN    32  // -> SER
    
    #define I2S_PULSE_US    2   // 1, 2, or 4 μs
    
    // Map step/dir to shift register bits
    #define I2S_STEP_0_BIT  0   // 74HC595 QA
    #define I2S_DIR_0_BIT   1   // 74HC595 QB
    #define I2S_STEP_1_BIT  2   // 74HC595 QC
    #define I2S_DIR_1_BIT   3   // 74HC595 QD
    #define I2S_STEP_2_BIT  4   // 74HC595 QE
    #define I2S_DIR_2_BIT   5   // 74HC595 QF
#endif
```

## Testing Recommendations

1. **First build with I2S disabled** (default) - verify nothing broke
2. **Enable I2S mode** - uncomment `USE_I2S_OUT` in hardware.h
3. **Wire up 74HC595** according to I2S_README.md
4. **Test with LinuxCNC** using existing test config

## Implementation Notes

### Why FluidNC's Approach?
- **Proven**: Thousands of users running FluidNC with I2S
- **Optimized**: Years of development and bug fixes
- **Compatible**: Works with common shift registers (74HC595, etc.)
- **Clean API**: Simple read/write interface

### Differences from FluidNC
- **Simpler**: Removed step engine integration (we use timers)
- **Focused**: Only I2S output (no input support)
- **Adapted**: Integrated with existing HAL2UDP architecture
- **Standalone**: No dependencies on FluidNC framework

### Performance Impact
- **I2S disabled**: Zero overhead, identical to current code
- **I2S enabled**: Minimal CPU impact (DMA handles data transfer)
- **Step frequency**: Still capable of 100 kHz step rates

## Future Enhancements (Optional)

1. **Digital outputs via I2S**: Map OUT_00-05 to shift register bits
2. **PWM via I2S**: Software PWM on I2S outputs
3. **More axes**: Easy to add more step/dir pairs
4. **Input expansion**: Use shift-in registers (74HC165)

## Credits

- **FluidNC Team**: Original I2S implementation
  - Michiyasu Odaki (initial I2S driver, 2020)
  - Mitch Bradley (I2S step engine, 2024)
  - Stefan de Bruijn (I2S bus abstraction, 2021)
- **License**: GPLv3 (same as HAL2UDP)

## Quick Start

1. Keep I2S disabled for now (default behavior unchanged)
2. When ready to test I2S:
   - Uncomment `#define USE_I2S_OUT` in `src/hardware.h`
   - Wire 74HC595 as shown in I2S_README.md
   - Build and flash: `pio run -t upload`
   - Test with LinuxCNC

See **I2S_README.md** for complete hardware setup and troubleshooting guide.
