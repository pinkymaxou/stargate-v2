# Stargate Mainboard ESP32

A fully functional Stargate replica controller based on ESP32, designed to replace Raspberry Pi-based solutions with a more cost-effective and integrated approach.

[![CI](https://github.com/pinkymaxou/stargate-v2/actions/workflows/ci.yml/badge.svg)](https://github.com/pinkymaxou/stargate-v2/actions/workflows/ci.yml)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v6.1--dev-blue)
![License](https://img.shields.io/badge/license-See%20doc%2Fcopyright.md-lightgrey)

## Overview

This project provides a complete embedded system for controlling Stargate props designed by Kristian and the "Build The Stargate" community. The ESP32-based solution offers:

- **Lower Cost**: ESP32 modules are significantly cheaper than Raspberry Pi
- **Better Availability**: No shortage issues
- **Integrated Design**: All drivers, regulators, and control circuitry on a single PCB
- **Simpler Wiring**: Fewer external breakouts required
- **Real-Time Control**: FreeRTOS-based deterministic motor control

## Features

### ✅ Implemented

#### Core Functionality
- ✅ **Web Interface**: Full-featured HTTP server with REST API
- ✅ **WiFi**: Station + SoftAP dual-mode support
- ✅ **OTA Updates**: Over-the-air firmware updates
- ✅ **Configuration Management**: JSON import/export
- ✅ **Multiple Gate Types**: Movie/SG1, Atlantis, Universe support

#### Hardware Control
- ✅ **Ring Motor Control**: Stepper motor with precise positioning
- ✅ **Chevron Control**: 9 chevron LED lighting (configurable for 7)
- ✅ **Servo Control**: Chevron locking mechanism (MCPWM-based)
- ✅ **Wormhole LEDs**: 48 NeoPixel LED effects
- ✅ **Ramp Lighting**: PWM-controlled LED brightness
- ✅ **Audio System**: Integrated amplifier with sound effects

#### Advanced Features
- ✅ **Auto-Homing**: Hall sensor-based ring calibration
- ✅ **Auto-Calibration**: Automatic steps-per-rotation detection
- ✅ **BLE Communication**: Wireless control of smart ring LEDs
- ✅ **Address Book**: Hardcoded and fan gate addresses
- ✅ **Incoming Wormholes**: Support for remote gate dialing
- ✅ **Diagnostic Mode**: Hardware testing and troubleshooting

### 🚧 In Progress

- 🚧 **DHD Board Support**: Integration with STM32-based DHD
- 🚧 **SD Card Support**: External storage for audio assets

**Note**: Pablo Board HAL is not yet ready to work and should not be used.

### 📋 Planned

- 📋 **Subspace Network**: Connection to other fan gates
- 📋 **Mobile App**: Dedicated iOS/Android control
- 📋 **Voice Control**: Smart home integration
- 📋 **Advanced Wormhole Effects**: Blackhole, unstable wormhole

## Quick Start

### Prerequisites

- **Hardware**: ESP32-S3 mainboard (pinky-board or compatible)
- **Software**: ESP-IDF 5.3.1
- **Tools**: Python 3.8+, CMake, Ninja

### Installation

1. **Install ESP-IDF 5.3**:
```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.3.1 --recursive https://github.com/espressif/esp-idf.git esp-idf-5.3
cd esp-idf-5.3
./install.sh
```

2. **Clone Repository**:
```bash
git clone https://github.com/user/stargate-mainboard-esp32.git
cd stargate-mainboard-esp32
```

3. **Build and Flash**:
```bash
cd firmware/stargate-fw/pinky-board
. ~/esp/esp-idf-5.3/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### First-Time Setup

1. Power on the ESP32
2. Connect to WiFi network `Stargate-Universe`
3. Navigate to `http://192.168.4.1`
4. Follow the setup wizard
5. Configure WiFi and gate type
6. Run homing sequence

## Architecture

The system consists of multiple firmware components:

- **stargate-fw**: Main controller (ESP32-S3)
  - Gate control state machine
  - Web server and REST API
  - WiFi and BLE management
  - Motor control and sensor reading

- **ring-fw**: Smart ring controller (ESP32)
  - BLE server for receiving commands
  - 48 NeoPixel LED control
  - Animation effects and power management

- **ring-factory**: Factory test firmware
  - Hardware diagnostics
  - Burn-in testing
  - Factory reset capability

For detailed architecture information, see [doc/architecture.md](doc/architecture.md).

## Documentation

Comprehensive documentation is available in the [doc/](doc/) directory:

### Essential Reading
- [📖 Architecture](doc/architecture.md) - System design and components
- [🔌 BLE Protocol](doc/ble-protocol.md) - Ring communication details
- [🌐 API Reference](doc/api-reference.md) - HTTP REST API
- [🏠 Homing Algorithm](doc/homing-algorithm.md) - Ring calibration
- [📞 Dial Algorithm](doc/dial-algorithm.md) - Symbol positioning math

### User Guides
- [🎮 Control Page](doc/control-page.md) - Operating the gate
- [⚙️ Configuration](doc/configuration-page.md) - Settings management
- [🔧 Diagnostic Page](doc/diagnostic-page.md) - Troubleshooting
- [📦 OTA Updates](doc/ota-page.md) - Firmware updates

### Technical Details
- [🌌 Stargate Types](doc/stargate-types.md) - Gate specifications
- [🌀 Wormhole Types](doc/wornhole-types.md) - Visual effects
- [📥 Incoming Wormhole](doc/incoming-wormhole.md) - Receiving dials
- [💾 Partitions](doc/partitions.md) - Flash memory layout

## Hardware

### Pinky Board (Production)

**Specifications**:
- MCU: ESP32-S3-WROOM-1
- Flash: 16 MB
- PSRAM: 8 MB
- WiFi: 802.11 b/g/n
- Bluetooth: BLE 5.0

**Connections**:
```
Stepper Motor:  GPIO 25 (Step), GPIO 33 (Dir), GPIO 26 (Sleep)
Servo Motor:    GPIO 4 (MCPWM)
Hall Sensor:    GPIO 32 (Input)
Wormhole LEDs:  GPIO 27 (WS2812B - 48 LEDs)
Ramp LED:       GPIO 23 (PWM)
Status LED:     GPIO 5
```

### Ring Board

**Specifications**:
- MCU: ESP32-WROOM-32
- Flash: 4 MB
- BLE: Server mode

**Connections**:
```
NeoPixel Ring:  GPIO 13 (48 LEDs in 4 concentric rings)
```

## API Examples

### Dial an Address
```bash
curl -X POST http://stargate.local/api/gate/dial \
  -H "Content-Type: application/json" \
  -d '{
    "address": [27, 7, 15, 32, 12, 30, 1],
    "galaxy": "milky_way"
  }'
```

### Get Status
```bash
curl http://stargate.local/api/system/status
```

### Control Ring LEDs
```bash
curl -X POST http://stargate.local/api/ring/animation \
  -H "Content-Type: application/json" \
  -d '{"animation": "fade_in"}'
```

For complete API documentation, see [doc/api-reference.md](doc/api-reference.md).

## Development

### Project Structure
```
stargate-mainboard-esp32/
├── firmware/
│   ├── stargate-fw/        # Main controller
│   │   ├── main-app/       # Application logic
│   │   └── pinky-board/    # Hardware implementation
│   ├── ring-fw/            # Ring controller
│   └── ring-factory/       # Factory test firmware
├── esp32-components/       # Shared utilities
│   └── misc-formula/       # Math functions
├── sgc-components/         # Stargate-specific
│   └── sgu-ringcomm/       # BLE protocol
└── doc/                    # Documentation
```

### Building Different Targets

**Main Controller**:
```bash
cd firmware/stargate-fw/pinky-board
idf.py build
```

**Ring Firmware**:
```bash
cd firmware/ring-fw
idf.py build
```

**Factory Test**:
```bash
cd firmware/ring-factory
idf.py build
```

### Code Style

- C++ Standard: C++17
- Naming: PascalCase for classes, camelCase for methods
- Indentation: 4 spaces
- Comments: Doxygen format for APIs

## Why ESP32 over Raspberry Pi?

| Feature | ESP32 | Raspberry Pi |
|---------|-------|--------------|
| Cost | ~$5-10 | ~$35-75 |
| Availability | Readily available | Frequent shortages |
| Real-time Control | Yes (FreeRTOS) | Limited (Linux) |
| Power Consumption | ~80mA | ~500mA+ |
| Boot Time | <1 second | 20-40 seconds |
| Integration | Single PCB possible | Requires breakouts |
| GPIO | 34 pins | 28 pins (40-pin header) |
| Reliability | No SD card corruption | SD card wear issues |

## Troubleshooting

### Build Issues

**MCPWM API errors**:
- Ensure ESP-IDF 5.3+ is installed
- Check compatibility notes in [doc/architecture.md](doc/architecture.md)

**BLE compilation errors**:
- Add required NimBLE headers
- Include `#undef min` / `#undef max` after includes

**IRAM overflow**:
- Set `CONFIG_COMPILER_OPTIMIZATION_SIZE=y` in sdkconfig

### Runtime Issues

**Ring won't connect**:
- Verify ring is powered
- Check BLE is enabled
- Check logs: `idf.py monitor`

**Stepper doesn't move**:
- Check sleep pin (GPIO 26) is HIGH
- Verify power supply to stepper driver
- Test with manual step command

**Homing fails**:
- Check hall sensor wiring (GPIO 32)
- Run manual homing via web interface
- Verify sensor triggers when magnet is near

## Contributing

Contributions are welcome! Please:

1. Read the documentation in [doc/](doc/)
2. Follow the code style guidelines
3. Test thoroughly on hardware
4. Submit pull requests with clear descriptions

## Community

- **Forum**: [Build The Stargate Community](https://buildthestargateproject.com/)
- **Discord**: [Stargate Builders](https://discord.gg/stargate)
- **YouTube**: [Kristian's Channel](https://youtube.com/kristian)

## License

See [doc/copyright.md](doc/copyright.md) for licensing information.

## Acknowledgments

- **Kristian**: Original Stargate design and inspiration
- **Build The Stargate Project**: Community support
- **Espressif**: ESP-IDF framework
- **Contributors**: Everyone who helped make this possible

---

**Current Version**: 1.0.0
**ESP-IDF Version**: 5.3.1
**Last Updated**: 2026-01-22

For detailed documentation, visit the [doc/](doc/) directory.
