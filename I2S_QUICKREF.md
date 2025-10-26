# I2S Quick Reference Card

## 🎯 What Is This?

Optional feature to drive step/dir signals through shift registers (74HC595) instead of direct GPIO pins.

**Benefits**: Use 3 pins instead of 6, expandable to 32+ outputs, hardware-synchronized.

## 🔧 Quick Enable (3 Steps)

1. **Edit** `src/hardware.h`:
   ```c
   #define USE_I2S_OUT  // Uncomment this line
   ```

2. **Wire** 74HC595:
   ```
   ESP32 GPIO 26 -> 74HC595 Pin 12 (RCLK)
   ESP32 GPIO 27 -> 74HC595 Pin 11 (SRCLK)
   ESP32 GPIO 32 -> 74HC595 Pin 14 (SER)
   ```

3. **Build & Flash**:
   ```bash
   pio run -t upload
   ```

## 📋 Default Pin Mapping

| Axis | Step Bit | Dir Bit | 74HC595 Pin |
|------|----------|---------|-------------|
| X    | 0        | 1       | QA, QB      |
| Y    | 2        | 3       | QC, QD      |
| Z    | 4        | 5       | QE, QF      |

## ⚙️ Configuration Options

In `src/hardware.h`:

```c
#define USE_I2S_OUT            // Enable I2S mode

#ifdef USE_I2S_OUT
    // I2S Control Pins
    #define I2S_WS_PIN      26    // Latch clock
    #define I2S_BCK_PIN     27    // Shift clock
    #define I2S_DATA_PIN    32    // Serial data
    
    // Timing (1, 2, or 4 microseconds)
    #define I2S_PULSE_US    2
    
    // Bit Assignments (0-31)
    #define I2S_STEP_0_BIT  0     // X axis step
    #define I2S_DIR_0_BIT   1     // X axis dir
    #define I2S_STEP_1_BIT  2     // Y axis step
    #define I2S_DIR_1_BIT   3     // Y axis dir
    #define I2S_STEP_2_BIT  4     // Z axis step
    #define I2S_DIR_2_BIT   5     // Z axis dir
#endif
```

## 🔌 74HC595 Minimal Wiring

```
74HC595 Pinout:
┌────────────────┐
│  16-VCC        │ -> 3.3V or 5V
│  15-QA (bit 0) │ -> STEP 0
│  14-SER        │ <- ESP32 GPIO 32 (DATA)
│  13-OE         │ -> GND
│  12-RCLK       │ <- ESP32 GPIO 26 (WS)
│  11-SRCLK      │ <- ESP32 GPIO 27 (BCK)
│  10-SRCLR      │ -> VCC
│   9-Q7'        │ -> Next IC (if chaining)
│   8-GND        │ -> GND
│   7-QH (bit 7) │
│   6-QG (bit 6) │
│   5-QF (bit 5) │ -> DIR 2
│   4-QE (bit 4) │ -> STEP 2
│   3-QD (bit 3) │ -> DIR 1
│   2-QC (bit 2) │ -> STEP 1
│   1-QB (bit 1) │ -> DIR 0
└────────────────┘
```

## 🚨 Common Issues

| Problem | Solution |
|---------|----------|
| No outputs | Check OE (pin 13) is connected to GND |
| Random data | Connect SRCLR (pin 10) to VCC |
| Wrong pins | Verify bit mappings in hardware.h |
| Too slow | Change I2S_PULSE_US to 1 |
| Too fast | Change I2S_PULSE_US to 4 |

## 📊 Performance

| Mode   | CPU Usage | Max Step Freq | Pins Used | Expandable |
|--------|-----------|---------------|-----------|------------|
| GPIO   | Low       | 100 kHz       | 6         | No         |
| I2S    | Very Low  | 100 kHz       | 3         | Yes (32+)  |

## ⚠️ Important Notes

1. **Pin Conflicts**: Default I2S pins (26, 27, 32) conflict with inputs IN_00, IN_01, IN_02
   - Solution: Use freed GPIO 12,13,16,17,21,22 for I2S instead
   
2. **Timing**: I2S adds ~2-8 μs latency (vs ~0.1 μs for direct GPIO)
   - This is negligible for CNC applications
   
3. **Outputs Only**: I2S mode is output-only (no input expansion yet)

4. **Compile Time**: Mode is selected at compile time (not runtime)

## 📚 Full Documentation

- **I2S_README.md**: Complete hardware setup guide
- **I2S_IMPLEMENTATION.md**: Technical implementation details
- **I2S_DIAGRAMS.md**: Visual diagrams and signal flows

## 🧪 Testing Checklist

- [ ] Compile with I2S disabled (verify no breaking changes)
- [ ] Enable I2S in hardware.h
- [ ] Compile with I2S enabled (should build without errors)
- [ ] Wire 74HC595 according to diagram
- [ ] Power up and check for smoke (kidding... mostly)
- [ ] Test with LinuxCNC test config
- [ ] Verify step pulses with oscilloscope
- [ ] Run test program and check feedback

## 💡 Tips

- Start with default settings (2 μs pulse width)
- Use pull-down resistors on 74HC595 outputs (10k)
- For 5V steppers: Power 74HC595 with 5V
- For 3.3V steppers: Power 74HC595 with 3.3V
- Chain multiple ICs for more axes (connect Q7' to next SER)
- Can mix I2S outputs with direct GPIO inputs

## 🔄 Switch Back to GPIO Mode

Simply comment out in `hardware.h`:
```c
// #define USE_I2S_OUT
```
Then rebuild. All step/dir signals return to direct GPIO.

## 🆘 Need Help?

Check the detailed docs:
1. **Basic setup**: Read I2S_README.md
2. **Wiring help**: See diagrams in I2S_DIAGRAMS.md  
3. **How it works**: Read I2S_IMPLEMENTATION.md
4. **LinuxCNC side**: No changes needed to LinuxCNC config

---
Based on FluidNC I2S implementation | GPLv3 License
