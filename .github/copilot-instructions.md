# Copilot Instructions for HAL2UDP

## Project Overview
- HAL2UDP is an external step generator and IO interface for LinuxCNC, communicating over Ethernet using ESP32 and W5500 modules.
- The system operates in velocity mode and is designed for real-time CNC control.
- Supports both direct GPIO and I2S shift register modes for step/dir signals.
- Key directories: 
  - `src/`: ESP32 firmware source code (C, ESP-IDF, native W5500 driver)
  - `LinuxCNC driver/`: LinuxCNC HAL driver (main: `udp.comp`)
  - `LinuxCNC test config/udp-test/`: Example/test LinuxCNC configuration

## Architecture & Data Flow
- ESP32 firmware (in `src/`) handles step/dir signals, IO, and Ethernet communication via W5500.
- LinuxCNC communicates with the ESP32 via UDP using the custom HAL component (`udp.comp`).
- Data flows: LinuxCNC HAL pins/parameters → UDP packets → ESP32 hardware control.

## Developer Workflows
- **Build ESP32 firmware:** Use PlatformIO (see `platformio.ini`).
- **Build/install LinuxCNC driver:**
  - Install dependencies: `sudo apt-get install linuxcnc-uspace-dev build-essential`
  - Compile/install HAL component: `sudo halcompile --install udp.comp`
- **Test with LinuxCNC:** Use configs in `LinuxCNC test config/udp-test/`.

## Project-Specific Conventions
- All communication is via UDP, no external libraries for W5500 (native driver in `src/w5500.c`).
- HAL pin/parameter naming: `udp.stepgen.#`, `udp.out.#`, `udp.pwm.#`, `udp.in.#`, `udp.ready`, `udp.enable`, `udp.lost`.
- PWM and digital outputs share pins; only one mode active per pin (see README PWM usage).
- Hardware pin mapping is fixed (see README Hardware section).
- **I2S Mode**: Optionally use I2S peripheral to drive step/dir pins via shift registers (e.g., 74HC595).
  - Enable by uncommenting `#define USE_I2S_OUT` in `src/hardware.h`
  - Configure I2S pins (WS, BCK, DATA) and bit mappings in `src/hardware.h`
  - Based on FluidNC's I2S implementation (`src/i2s_out.c`, `src/i2s_out.h`)

## Integration Points
- LinuxCNC <-> ESP32 via UDP (custom protocol, see `udp.comp` and `src/comm.h`).
- ESP32 <-> W5500 via SPI (see `src/w5500.c`, `src/w5500.h`).

## Key Files
- `src/main.c`: ESP32 firmware entry point
- `src/w5500.c/h`: W5500 Ethernet driver
- `src/stepgen.h`: Step generation logic
- `src/i2s_out.c/h`: I2S shift register driver (optional, FluidNC-based)
- `src/hardware.h`: Hardware pin mappings and I2S configuration
- `LinuxCNC driver/udp.comp`: LinuxCNC HAL component
- `LinuxCNC test config/udp-test/udp-test.hal`: Example HAL config

## Example: Adding a HAL Pin
- Define pin in `udp.comp` (LinuxCNC driver)
- Map to UDP protocol in `src/comm.h`/`src/main.c`
- Handle in ESP32 logic (update hardware state)

## Tips for AI Agents
- Always check both LinuxCNC and ESP32 sides for changes to communication or pin mapping.
- Use PlatformIO for ESP32 builds; use `halcompile` for LinuxCNC driver.
- Follow existing pin/parameter naming conventions for new features.
- Reference hardware mapping in README for GPIO assignments.

---

*If any section is unclear or missing, please provide feedback for further refinement.*
