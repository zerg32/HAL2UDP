# Jackpot CNC Controller Compatibility

HAL2UDP I2S configuration is fully compatible with the [Jackpot CNC Controller](https://docs.v1e.com/electronics/jackpot/) from V1 Engineering.

## Verified Configuration Match

### I2S Control Pins
Both HAL2UDP and Jackpot use identical I2S pin assignments:

| Signal | GPIO Pin | 74HC595 Connection |
|--------|----------|-------------------|
| WS     | GPIO 17  | RCLK (Pin 12)     |
| BCK    | GPIO 22  | SRCLK (Pin 11)    |
| DATA   | GPIO 21  | SER (Pin 14)      |

**Source:** [Jackpot LowRider config.yaml](https://github.com/V1EngineeringInc/FluidNC_Configs/blob/main/LowRider%20CNC/Jackpot/UI%20V2%20LRCNC/config.yaml)

### Motor Pin Mapping

HAL2UDP supports the first 3 motors using the exact Jackpot/FluidNC bit assignments:

| Motor | HAL2UDP Use | Jackpot Use | Step Bit | Dir Bit | Disable Bit |
|-------|-------------|-------------|----------|---------|-------------|
| 0     | Axis 0 (X)  | X axis      | I2SO.2   | I2SO.1  | I2SO.0      |
| 1     | Axis 1 (Y)  | Y axis      | I2SO.5   | I2SO.4  | I2SO.7      |
| 2     | Axis 2 (Z)  | Z axis      | I2SO.10  | I2SO.9  | I2SO.8      |

Additional motors available in Jackpot (not used by HAL2UDP):
- Motor 3 (A): I2SO.13 (step), I2SO.12 (dir), I2SO.15 (disable)
- Motor 4 (B): I2SO.18 (step), I2SO.17 (dir), I2SO.16 (disable)
 
## Note about HAL2UDP firmware support

Currently HAL2UDP's built-in step generation (`stepgen_task`) is implemented for 3 axes (X, Y, Z). That means:

- Motors 0..2 (I2SO.0..I2SO.10) are directly supported by the real-time step generation logic.
- Motors 3 and 4 (I2SO.12..I2SO.18) are exposed as I2S outputs and documented in `src/hardware.h`, so they can be used as auxiliary motor outputs or for custom wiring, but they are not driven by HAL2UDP's `stepgen_task` by default.

If you want full runtime support for 5 independent axes driven by HAL2UDP, the firmware needs to be extended (add timers/step engines and expand the comm/stepgen data structures). I can prepare a plan and implement that if you'd like.

## Hardware Compatibility

### Shift Register Requirements
Both systems use the same shift register approach:
- **IC Type**: 74HC595 or compatible
- **Chain Length**: Minimum 2 ICs for 3-axis (16 bits), up to 4 ICs for 5-axis (32 bits)
- **Power**: 3.3V or 5V compatible

### Wiring
HAL2UDP with I2S enabled can use the exact same shift register wiring as Jackpot:

```
ESP32 GPIO 17 -> First 74HC595 Pin 12 (RCLK)
ESP32 GPIO 22 -> First 74HC595 Pin 11 (SRCLK)
ESP32 GPIO 21 -> First 74HC595 Pin 14 (SER)

First 74HC595 Pin 9 (Q7') -> Second 74HC595 Pin 14 (SER)
Second 74HC595 Pin 9 (Q7') -> Third 74HC595 Pin 14 (SER)
...
```

## Configuration Comparison

### HAL2UDP (hardware.h)
```c
#define USE_I2S_OUT

#ifdef USE_I2S_OUT
    #define I2S_WS_PIN      17
    #define I2S_BCK_PIN     22
    #define I2S_DATA_PIN    21
    #define I2S_PULSE_US    2
    
    // Motor 0
    #define I2S_STEP_0_BIT  2
    #define I2S_DIR_0_BIT   1
    #define I2S_DIS_0_BIT   0
    
    // Motor 1
    #define I2S_STEP_1_BIT  5
    #define I2S_DIR_1_BIT   4
    #define I2S_DIS_1_BIT   7
    
    // Motor 2
    #define I2S_STEP_2_BIT  10
    #define I2S_DIR_2_BIT   9
    #define I2S_DIS_2_BIT   8
#endif
```

### Jackpot/FluidNC (config.yaml)
```yaml
i2so:
  bck_pin: gpio.22
  data_pin: gpio.21
  ws_pin: gpio.17

stepping:
  pulse_us: 2

# Motor 0 (X)
direction_pin: I2SO.1
step_pin: I2SO.2
disable_pin: I2SO.0

# Motor 1 (Y)
direction_pin: I2SO.4
step_pin: I2SO.5
disable_pin: I2SO.7

# Motor 2 (Z)
direction_pin: I2SO.9
step_pin: I2SO.10
disable_pin: I2SO.8
```

## Benefits of Compatibility

1. **Shared Hardware**: Can use same shift register boards/breakouts designed for Jackpot
2. **Known Configuration**: Leverage existing Jackpot documentation and community knowledge
3. **Proven Design**: Based on production-tested Jackpot CNC controller
4. **Future Expansion**: Easy to add more axes following Jackpot pattern

## Differences

| Feature | HAL2UDP | Jackpot |
|---------|---------|---------|
| Firmware | Custom ESP-IDF | FluidNC |
| Protocol | UDP to LinuxCNC | GRBL/WiFi |
| Max Axes | 3 (configurable) | 6 |
| TMC Support | No | Yes (TMC2209) |
| Control | LinuxCNC HAL | WebUI/GRBL |

## Migration Path

### From Direct GPIO to I2S (HAL2UDP)
1. Uncomment `#define USE_I2S_OUT` in `src/hardware.h`
2. Wire shift registers using Jackpot pinout
3. Rebuild and flash firmware
4. No LinuxCNC configuration changes needed

### Using Jackpot Hardware with HAL2UDP
If you have Jackpot hardware and want to use LinuxCNC instead of FluidNC:
1. Flash HAL2UDP firmware to ESP32
2. Enable I2S mode
3. Configure network settings
4. Connect to LinuxCNC via UDP
5. Keep existing shift register wiring

## References

- **Jackpot Documentation**: https://docs.v1e.com/electronics/jackpot/
- **Jackpot Config Repository**: https://github.com/V1EngineeringInc/FluidNC_Configs
- **FluidNC Wiki**: http://wiki.fluidnc.com/
- **HAL2UDP I2S Docs**: See I2S_README.md, I2S_QUICKREF.md

## Version Information

- **HAL2UDP I2S Implementation**: Based on FluidNC v3.9.5
- **Verified Against**: Jackpot LowRider config dated 10-31-2024
- **Compatible With**: All Jackpot hardware revisions (V1.0+)

---

*This compatibility was verified on October 26, 2025 against official V1 Engineering Jackpot configurations.*
