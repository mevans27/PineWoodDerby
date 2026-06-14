# Pinewood Derby Timer

A 4-lane Pinewood Derby race timer built on the ESP8266 (NodeMCU/Wemos D1 Mini). It detects lane finishes with millisecond precision, displays results on a small OLED screen, serves a live web dashboard over WiFi, and stores race history for CSV download.

## Features

- 4-lane finish detection with millisecond accuracy
- Start gate sensor for automatic race triggering
- Winner LED indicator (lights up the winning lane)
- 128x64 OLED display with scrolling results
- Built-in WiFi access point (no router needed)
- Live web dashboard with auto-refresh
- CSV export of all race history
- Automatic race timeout (5 seconds after first finish)
- Stores up to 50 races in memory

## Hardware

### Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP8266 board (NodeMCU or Wemos D1 Mini) | 1 | Main microcontroller |
| 128x64 I2C OLED display (SSD1306) | 1 | Results display |
| IR break-beam sensors (or micro switches) | 5 | 1 start gate + 4 lane finish sensors |
| LEDs | 4 | Winner indicator per lane |
| Resistors (220Ω) | 4 | Current limiting for LEDs |
| Jumper wires | As needed | |

### Pin Wiring

| Function | Pin | Notes |
|----------|-----|-------|
| Start Gate Sensor | D8 | Triggers race start |
| Lane 1 Finish Sensor | D7 | |
| Lane 2 Finish Sensor | D6 | |
| Lane 3 Finish Sensor | D5 | |
| Lane 4 Finish Sensor | D0 | |
| Lane 1 LED | GPIO 1 | Uses TX pin (serial disabled) |
| Lane 2 LED | GPIO 3 | Uses RX pin (serial disabled) |
| Lane 3 LED | D4 | |
| Lane 4 LED | D3 | |
| OLED SDA | D2 | I2C data |
| OLED SCL | D1 | I2C clock |

> **Note:** Serial communication is disabled because the TX and RX pins are repurposed for Lane 1 and Lane 2 LEDs. You won't be able to use the Serial Monitor while LEDs are connected.

### Sensor Wiring

All sensors (start gate and lane finish) use `INPUT_PULLUP`, meaning:
- **HIGH (default):** No car detected (beam unbroken / switch open)
- **LOW (triggered):** Car detected (beam broken / switch closed)

Wire each sensor between its designated pin and GND. The internal pull-up resistor holds the pin HIGH until the sensor pulls it LOW.

### OLED Display

Connect using I2C:
- SDA → D2
- SCL → D1
- VCC → 3.3V
- GND → GND

The display uses I2C address `0x3C`.

## Physical Setup

1. **Mount finish sensors** at the end of each lane, positioned so a passing car breaks the beam or triggers the switch.
2. **Mount the start gate sensor** at the top of the track where the starting gate drops. When the gate opens (or the car passes), the sensor goes LOW to trigger the race.
3. **Mount LEDs** near each lane's finish line so spectators can see which car won.
4. **Mount the OLED display** where the race operator can see it.
5. **Power the ESP8266** via USB (phone charger or battery pack works fine).

### Race Flow

1. Power on the device — OLED shows "READY"
2. The system is armed and waiting for the start gate sensor
3. When the start gate opens (sensor goes LOW), the timer starts and the OLED shows "GO!"
4. As each car crosses its lane sensor, the time is recorded
5. The first car to finish triggers its lane LED
6. The race ends when all 4 lanes finish OR 5 seconds after the first finish
7. Results scroll on the OLED (page 1: Lanes 1-2, page 2: Lanes 3-4)
8. Trigger the start gate sensor again to reset and arm for the next race

> **Timeout:** If a car never finishes, it's recorded as 99.999 seconds after the 5-second timeout.

## Web Application

### Connecting

1. On your phone, tablet, or laptop, open WiFi settings
2. Connect to the network:
   - **SSID:** `Pinewood_Timer`
   - **Password:** `thereisnospoon`
3. Open a web browser and go to: **http://192.168.4.1**

### Dashboard

The web dashboard shows:
- Current lane times (or "---" for lanes that haven't finished)
- Auto-refreshes every 1 second during a race, every 5 seconds otherwise
- A link to download all race results as a CSV file

### CSV Download

Click the "Download CSV" link on the dashboard to get a file containing:
- Race number
- Lane 1-4 times (in seconds)
- Winner for each race

Example CSV output:
```
Race,L1,L2,L3,L4,Winner
1,2.341,2.567,2.123,2.890,Lane 3
2,2.456,2.234,2.678,2.345,Lane 2
```

## Software Setup

### Dependencies

Install these libraries in the Arduino IDE (Sketch → Include Library → Manage Libraries):

- `ESP8266WiFi` (included with ESP8266 board package)
- `ESP8266WebServer` (included with ESP8266 board package)
- `Adafruit GFX Library`
- `Adafruit SSD1306`

### Board Setup

1. In Arduino IDE, go to File → Preferences
2. Add this URL to "Additional Board Manager URLs":
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Go to Tools → Board → Board Manager, search for "ESP8266" and install
4. Select your board (e.g., "NodeMCU 1.0" or "LOLIN(WEMOS) D1 Mini")
5. Select the correct COM port

### Upload

1. Open `PinewoodDerby.ino` in Arduino IDE
2. Click Upload
3. Once uploaded, the OLED should display "READY"

## Configuration

These values can be changed in the source code:

| Setting | Default | Description |
|---------|---------|-------------|
| `APSSID` | `Pinewood_Timer` | WiFi network name |
| `APPSK` | `thereisnospoon` | WiFi password |
| `MAX_HISTORY` | `50` | Maximum races stored in memory |
| Timeout | 5000 ms | Time after first finish before race ends |

## Troubleshooting

| Problem | Solution |
|---------|----------|
| OLED blank | Check I2C wiring (SDA/SCL), verify address is 0x3C |
| Can't find WiFi network | Make sure ESP8266 is powered; try restarting |
| Race doesn't start | Check start gate sensor wiring; sensor should go LOW when triggered |
| Lane times not recording | Verify sensor is pulling pin LOW when car passes |
| LEDs not lighting | Check LED polarity and resistor; remember TX/RX pins are used |
| Can't upload code | Disconnect LEDs from TX/RX pins before uploading |

## License

MIT
