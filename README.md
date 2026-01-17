# ESPHome Gree AC Component

A modern, feature-rich ESPHome component for Gree/Sinclair air conditioners with UART communication.

## Features

- ✅ **Full climate control** — Temperature, mode, fan speed, swing
- ✅ **Handshake retry** — Automatically retries connection if AC is unresponsive
- ✅ **Advanced swing control** — Separate horizontal and vertical position control
- ✅ **Extra features** — Plasma/Health, Sleep, X-Fan modes
- ✅ **Display control** — Turn AC display on/off
- ✅ **External sensor support** — Override AC's temperature sensor
- ✅ **Preset support** — Turbo/Boost mode
- ✅ **Modern ESPHome** — Compatible with latest ESPHome versions
- ✅ **Robust protocol** — Proper packet validation and checksums

## Supported Models

This component has been tested with:
- Gree air conditioners
- Sinclair air conditioners (MV-H09BIF and similar)
- Kentatsu Turin series
- Lessar Enigma series
- Lennox units (with minor modifications)

Most Gree-based AC units using the UART protocol should work.

## Installation

### Method 1: From GitHub (Recommended)

Fork the bekmansurov repo and use your fork:

```yaml
external_components:
  - source: github://yourusername/esphome_gree_hvac
    components: [ gree_ac ]
```

This allows you to easily pull updates and maintain your own modifications.

### Method 2: Copy to ESPHome config directory

1. Create a `components` folder in your ESPHome configuration directory
2. Copy the `gree_ac` folder into `components/`
3. Your structure should look like:
   ```
   esphome/
   ├── your-device.yaml
   └── components/
       └── gree_ac/
           ├── __init__.py
           ├── climate.py
           ├── gree_ac.h
           └── gree_ac.cpp
   ```

### Method 3: Local git submodule

```bash
cd esphome
git submodule add https://github.com/yourusername/esphome_gree_hvac components/gree_ac
```

## Hardware Connection

### Wiring Diagram

```
AC Unit Connector (4-pin)     ESP32/ESP8266
┌─────────────────┐          ┌──────────────┐
│ 1. +5V          │─────────>│ VIN/5V       │
│ 2. RX (AC)      │<─────────│ TX (GPIO17)  │
│ 3. TX (AC) 5V   │──[DIV]──>│ RX (GPIO16)  │
│ 4. GND          │─────────>│ GND          │
└─────────────────┘          └──────────────┘

[DIV] = Voltage divider: 2.2kΩ + 3.3kΩ resistors
        (5V from AC → 3.3V for ESP)
```

### Voltage Divider Circuit

```
     AC TX (5V)
         │
         ├──── 2.2kΩ ────┬──── ESP RX (3.3V)
         │               │
         └──── 3.3kΩ ────┴──── GND
```

### Safety Notes

⚠️ **IMPORTANT:**
- Disconnect AC power completely before working on connections
- The AC unit outputs 5V UART signals, ESP32/ESP8266 uses 3.3V
- **Always use a voltage divider** on the AC TX → ESP RX line
- The ESP TX → AC RX line is usually safe (3.3V is recognized by AC)

## Configuration

### Basic Configuration

```yaml
uart:
  tx_pin: GPIO17
  rx_pin: GPIO16
  baud_rate: 4800
  parity: EVEN
  data_bits: 8
  stop_bits: 1

climate:
  - platform: gree_ac
    name: "Living Room AC"
```

### Full Configuration with All Features

```yaml
climate:
  - platform: gree_ac
    id: my_ac
    name: "Living Room AC"
    update_interval: 1s
    
    # Optional: External temperature sensor
    current_temperature_sensor: room_temp_sensor
    
    # Optional: Enable turbo mode
    supported_presets:
      - BOOST
    
    # Advanced swing controls
    horizontal_swing_select:
      name: "Horizontal Swing"
    
    vertical_swing_select:
      name: "Vertical Swing"
    
    # Display control
    display_select:
      name: "Display"
    
    # Extra features
    plasma_switch:
      name: "Plasma/Health"
    
    sleep_switch:
      name: "Sleep Mode"
    
    xfan_switch:
      name: "X-Fan"
```

## Supported Features

### Climate Modes
- **Off** - Turn off the AC
- **Auto** - Automatic temperature control
- **Cool** - Cooling mode
- **Heat** - Heating mode
- **Dry** - Dehumidification mode
- **Fan Only** - Fan without heating/cooling

### Fan Speeds
- Auto
- Low
- Medium
- High
- Turbo (via preset)
- Quiet (if supported by your model)

### Swing Modes
- Off - Fixed position (center)
- Vertical - Swing up/down
- Horizontal - Swing left/right
- Both - Swing in all directions

### Advanced Swing Control

When using the optional swing selects, you get precise position control:

**Horizontal:** Off, Full Swing, Left, Mid-Left, Center, Mid-Right, Right
**Vertical:** Off, Full Swing, Up, Mid-Up, Center, Mid-Down, Down

### Presets
- **None** - Normal operation
- **Boost/Turbo** - Maximum cooling/heating power

### Extra Features
- **Plasma/Health** - Air purification (if supported)
- **Sleep** - Gradual temperature adjustment for sleeping
- **X-Fan** - Continue fan operation after cooling to dry evaporator
- **Display** - Control AC unit display on/off

## Troubleshooting

### AC Not Responding

The component implements automatic handshake retry:
- Retries connection every 5 seconds if AC is unresponsive
- Check UART wiring and voltage divider
- Verify baud rate is 4800 with EVEN parity
- Check ESPHome logs for communication errors

### Incorrect Temperature Readings

Use an external sensor:

```yaml
sensor:
  - platform: dallas_temp
    name: "Room Temperature"
    id: room_temp_sensor

climate:
  - platform: gree_ac
    current_temperature_sensor: room_temp_sensor
```

### Commands Not Working

- Ensure AC is in READY state (check logs)
- Verify voltage divider is working correctly
- Check for loose connections
- Some features may not be supported by all models

### Checksum Errors

- Usually indicates electrical noise or bad connections
- Try shorter wire lengths
- Add a 100nF capacitor between RX and GND
- Ensure proper grounding

## Protocol Details

The component uses a proprietary UART protocol:
- **Baud rate:** 4800
- **Parity:** Even
- **Data bits:** 8
- **Stop bits:** 1
- **Sync bytes:** 0x7E 0x7E
- **Packet structure:** [SYNC][SYNC][LENGTH][CMD][DATA...][CHECKSUM]

### Key Differences from Original Repos

This component combines the best features from both piotrva and bekmansurov repositories:

**From piotrva:**
- Handshake retry mechanism
- Advanced swing controls
- Display and extra feature switches
- More detailed protocol implementation

**From bekmansurov:**
- Cleaner packet parsing
- Preset support
- Polling component architecture

**New improvements:**
- Modern ESPHome structure (2024+)
- Better state management
- Improved error handling
- Rate limiting to prevent flooding AC
- Proper initialization sequence
- Enhanced logging

## Development

### Project Structure

```
gree_ac/
├── __init__.py      # Lightweight component registration
├── climate.py       # Platform configuration and schema (easy to fork/modify)
├── gree_ac.h        # C++ header with class definitions
└── gree_ac.cpp      # C++ implementation
```

**Why this structure?**
- `__init__.py` is minimal and rarely needs changes
- `climate.py` contains all the configuration logic - **fork and customize this!**
- C++ files contain the protocol implementation
- Easy to maintain as a fork of bekmansurov's repo

### Forking and Customizing

1. Fork bekmansurov's repository on GitHub
2. Replace the `components/gree` folder contents with these `gree_ac` files
3. Modify `climate.py` to add/remove features
4. Push your changes
5. Use in ESPHome:
   ```yaml
   external_components:
     - source: github://yourusername/esphome_gree_hvac
       components: [ gree_ac ]
   ```

### Testing

#### Unit Tests

The component includes comprehensive unit tests for protocol logic validation. These tests verify:
- Checksum calculation
- Temperature parsing (incoming/outgoing formulas)
- Mode and fan speed encoding/decoding
- Preset selection
- Swing mode mapping
- Packet structure validation
- DRY mode fan enforcement
- Indoor temperature parsing

**Run the unit tests:**

```bash
python3 tests/test_gree_ac_protocol.py
```

**Expected output:**
```
=== Gree AC Component Unit Tests ===

Test 1: Checksum Calculation
  ✓ Checksum calculation passed

Test 2: Temperature Parsing (Incoming)
  ✓ Temperature parsing passed

...

=== All tests passed! ===
```

#### Runtime Testing with ESPHome

Enable verbose logging in your config to see all UART communication:

```yaml
logger:
  level: VERBOSE
```

This will show all UART packets in hex format for debugging.

### Contributing

Contributions are welcome! If you have a different AC model or find issues:
1. Enable VERBOSE logging
2. Capture packet traces
3. Open an issue with model info and logs

For detailed information about contributing, including GitHub Copilot AI assistance options for developers, see [CONTRIBUTING.md](CONTRIBUTING.md).

## Credits

Based on the excellent work of:
- [piotrva/esphome_gree_ac](https://github.com/piotrva/esphome_gree_ac)
- [bekmansurov/esphome_gree_hvac](https://github.com/bekmansurov/esphome_gree_hvac)
- [DomiStyle/esphome-panasonic-ac](https://github.com/DomiStyle/esphome-panasonic-ac)

## License

GPL-3.0 License

## Disclaimer

⚠️ **USE AT YOUR OWN RISK**

This component involves replacing the stock WiFi module and working with AC electrical systems. Only proceed if you're comfortable with:
- Soldering and electronics
- Working with UART protocols
- Understanding the risks of modifying appliances

The authors are not responsible for any damage to your equipment or property.
