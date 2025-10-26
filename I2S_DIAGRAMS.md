# I2S Pin Mapping and Signal Flow

## Hardware Connection Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                         ESP32 Module                             │
│                                                                  │
│  ┌────────────────┐              ┌──────────────┐               │
│  │   W5500 SPI    │              │  I2S Periph  │               │
│  │                │              │   (Audio)    │               │
│  │  CS    <- GPIO5│              │              │               │
│  │  MISO  -> GPIO19│             │  WS   -> GPIO26              │
│  │  MOSI  <- GPIO23│             │  BCK  -> GPIO27              │
│  │  SCLK  <- GPIO18│             │  DATA -> GPIO32              │
│  └────────────────┘              └──────────────┘               │
│                                                                  │
│  GPIO 12,13,16,17,21,22 = Direct Step/Dir (default mode)        │
└──────────────────────────────────────────────────────────────────┘
           │                               │
           │ Direct GPIO Mode              │ I2S Mode
           │ (default)                     │ (optional)
           ▼                               ▼
    ┌─────────────┐              ┌──────────────────┐
    │  Stepper    │              │   74HC595        │
    │  Drivers    │              │ Shift Register   │
    │             │              │                  │
    │ STEP <- 12  │              │ RCLK  <- GPIO26  │
    │ DIR  <- 13  │              │ SRCLK <- GPIO27  │
    │ STEP <- 16  │              │ SER   <- GPIO32  │
    │ DIR  <- 17  │              │                  │
    │ STEP <- 21  │              │ QA -> STEP 0     │
    │ DIR  <- 22  │              │ QB -> DIR 0      │
    └─────────────┘              │ QC -> STEP 1     │
                                 │ QD -> DIR 1      │
                                 │ QE -> STEP 2     │
                                 │ QF -> DIR 2      │
                                 │ QG -> (unused)   │
                                 │ QH -> (unused)   │
                                 │                  │
                                 │ Q7' -> Next IC   │
                                 └──────────────────┘
```

## I2S Data Stream Format

```
32-bit word transmitted continuously via I2S DMA:

Bit Position:  31 30 29 28 27 26 ... 5  4  3  2  1  0
               ┌──┬──┬──┬──┬──┬──┬───┬──┬──┬──┬──┬──┬──┐
Shift Reg:     │ ?│ ?│ ?│ ?│ ?│ ?│...│D2│S2│D1│S1│D0│S0│
               └──┴──┴──┴──┴──┴──┴───┴──┴──┴──┴──┴──┴──┘
                                      │  │  │  │  │  │
                                      │  │  │  │  │  └─ I2S_STEP_0_BIT
                                      │  │  │  │  └──── I2S_DIR_0_BIT
                                      │  │  │  └─────── I2S_STEP_1_BIT
                                      │  │  └────────── I2S_DIR_1_BIT
                                      │  └───────────── I2S_STEP_2_BIT
                                      └──────────────── I2S_DIR_2_BIT

Key:
S0/S1/S2 = Step pins for axes 0, 1, 2
D0/D1/D2 = Direction pins for axes 0, 1, 2
? = Available for future use (digital outputs, more axes, etc.)
```

## Timing Diagram (I2S to 74HC595)

```
Time axis: ────────────────────────────────────────────────►

WS (RCLK):  ____┌──────────────────────────────┐____________
                │    32 bit clocks             │
                │                              │
BCK (SRCLK): ___┐_┌_┌_┌_┌_┌_┌_┌_┌_┌_┌_┌_┌_┌_┌_┐_┌_┌________
             ___│_│_│_│_│_│_│_│_│_│_│_│_│_│_│_│_│_│_│________
                │ 1 2 3 4 5 ...        ... 31 32│
                │                              │
DATA (SER):  ───┤b31 b30 b29...........b1  b0 ├────────────
                │                              │
                └──────────────────────────────┘
                          Shift in              └─ Latch to outputs

Pulse Width: Configurable via I2S_PULSE_US (1, 2, or 4 μs)
Update Rate: Continuous (I2S DMA keeps FIFO filled)
```

## Code Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                      app_main()                             │
│                         │                                   │
│                         ├─ io_init()                        │
│                         │                                   │
│                  ┌──────┴──────┐                            │
│                  │ USE_I2S_OUT?│                            │
│                  └──────┬──────┘                            │
│                         │                                   │
│           ┌─────────────┴─────────────┐                     │
│           │ YES                       │ NO                  │
│           ▼                           ▼                     │
│    ┌──────────────┐           ┌─────────────┐              │
│    │i2s_out_init()│           │  (skip)     │              │
│    │              │           └─────────────┘              │
│    │ - Setup I2S  │                                        │
│    │ - Config DMA │                                        │
│    │ - Route pins │                                        │
│    └──────────────┘                                        │
│           │                                                 │
│           └─────────────┬─────────────────────────────────►│
│                         ▼                                   │
│                  comm_task (core 0)                         │
│                  watchdog_task (core 0)                     │
│                  stepgen_task (core 1)                      │
│                         │                                   │
└─────────────────────────┼───────────────────────────────────┘
                          ▼
               ┌──────────────────────┐
               │   stepgen_task()     │
               │                      │
               │ ┌────────────────┐   │
               │ │ USE_I2S_OUT?   │   │
               │ └────┬───────────┘   │
               │      │               │
               │  NO  │  YES          │
               │  ┌───┴───┐           │
               │  │ Init  │  Skip     │
               │  │ GPIO  │  GPIO     │
               │  └───┬───┘           │
               │      │               │
               │      ▼               │
               │  Init Timers         │
               │      │               │
               │      ▼               │
               │  ┌─────────────┐    │
               │  │ Timer ISRs  │    │
               │  │             │    │
               │  │ STEP_x_H/L ────►─┼─┐
               │  │ DIR_x_H/L  ────►─┼─┤
               │  └─────────────┘    │ │
               └──────────────────────┘ │
                                        │
         ┌──────────────────────────────┘
         │
         ▼
    ┌────────────────────┐
    │ Conditional Macros │
    └────────────────────┘
         │
         ├─ USE_I2S_OUT defined:
         │  └─► i2s_out_write(bit, val)
         │      └─► Write to I2S FIFO
         │          └─► DMA to shift register
         │
         └─ USE_I2S_OUT not defined:
            └─► REGISTER_WRITE(GPIO_OUT_xxx, BITxx)
                └─► Direct GPIO register access
```

## Memory Layout

### Direct GPIO Mode (default)
- **Code size**: Original (no I2S code compiled)
- **RAM usage**: Original (no I2S buffers)
- **Performance**: Direct register writes (~1-2 CPU cycles)

### I2S Mode (enabled)
- **Code size**: +~4KB (i2s_out.c compiled in)
- **RAM usage**: +~256 bytes (I2S state + FIFO)
- **Performance**: Write to FIFO (~5-10 CPU cycles), DMA handles transfer
- **Latency**: ~2-8 μs (configurable via I2S_PULSE_US)

## Configuration Matrix

| Parameter       | Values           | Effect                          |
|----------------|------------------|---------------------------------|
| USE_I2S_OUT    | undefined (def.) | Direct GPIO mode                |
|                | defined          | I2S shift register mode         |
| I2S_PULSE_US   | 1                | 1 MHz clock, 1 μs pulse         |
|                | 2 (default)      | 500 kHz clock, 2 μs pulse       |
|                | 4                | 250 kHz clock, 4 μs pulse       |
| I2S_xxx_BIT    | 0-31             | Shift register bit mapping      |

## Pin Conflict Table

| GPIO | Default Use    | I2S Mode Use     | Notes                           |
|------|----------------|------------------|---------------------------------|
| 12   | STEP_0         | Available        | Not used in I2S mode            |
| 13   | DIR_0          | Available        | Not used in I2S mode            |
| 16   | STEP_1         | Available        | Not used in I2S mode            |
| 17   | DIR_1          | Available        | Not used in I2S mode            |
| 21   | STEP_2         | Available        | Not used in I2S mode            |
| 22   | DIR_2          | Available        | Not used in I2S mode            |
| 26   | IN_00 (input)  | I2S_WS (output)  | ⚠ Conflict! Reconfigure inputs  |
| 27   | IN_01 (input)  | I2S_BCK (output) | ⚠ Conflict! Reconfigure inputs  |
| 32   | IN_02 (input)  | I2S_DATA (output)| ⚠ Conflict! Reconfigure inputs  |

**Important**: Default I2S pins conflict with inputs IN_00, IN_01, IN_02. 
You can either:
1. Use different I2S pins (e.g., use the freed-up step/dir pins)
2. Reconfigure input pins to use different GPIOs
3. Reduce number of inputs if you don't need all 7
