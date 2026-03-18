# Flag Carriage

Self-driven flag carriage that rides along a fixed 6mm rope — controlled via WiFi from an iPhone app.

## Hardware

| Component | Spec |
|-----------|------|
| Microcontroller | ESP8266 NodeMCU v1.0 (replaces Arduino Nano) |
| Motor driver | L298N dual H-bridge |
| Motor | 12V DC gear motor, 100RPM, >=20kg.cm torque |
| Battery | 12V (drill battery or Li-ion pack) |
| Limit switches | 2x micro switches (optional — auto-reverse) |

## Repository structure

```
arduino/
  flag_carriage/
    flag_carriage.ino   <- Main sketch — upload to ESP8266 NodeMCU
    config.h            <- WiFi credentials and pin config — edit this first
ios/                    <- iOS Swift app (coming soon)
```

## Quick start

1. Install the ESP8266 board package in Arduino IDE
   - Preferences -> Additional Board URLs:
     `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
   - Tools -> Board Manager -> search "esp8266" -> install

2. Install the WebSockets library
   - Sketch -> Include Library -> Manage Libraries
   - Search: "WebSockets" by Links2004 -> install

3. Edit `config.h` with your WiFi details and choose your mode

4. Select board: **NodeMCU 1.0 (ESP-12E Module)**

5. Upload and open Serial Monitor at 115200 baud
   - It will print the IP address to connect to

## WiFi modes

| Mode | How it works | When to use |
|------|-------------|-------------|
| `WIFI_MODE_STA` | NodeMCU joins your router or iPhone hotspot | Fixed location with known network |
| `WIFI_MODE_AP` | NodeMCU creates "FlagCarriage" hotspot | Field use, no router available |

If STA mode fails to connect, it automatically falls back to AP mode.

## Wiring

```
L298N IN1  -> NodeMCU D1 (GPIO5)
L298N IN2  -> NodeMCU D2 (GPIO4)
L298N ENA  -> NodeMCU D5 (GPIO14)  [PWM]
Limit A    -> NodeMCU D6 (GPIO12)  [INPUT_PULLUP]
Limit B    -> NodeMCU D7 (GPIO13)  [INPUT_PULLUP]
L298N 12V  -> 12V battery +
L298N GND  -> Common GND
L298N 5V   -> NodeMCU Vin
```

**Do NOT connect 12V to NodeMCU directly. Use L298N 5V output -> NodeMCU Vin.**

## Command protocol

The iOS app connects via WebSocket to `ws://[IP]:81`

| Command | Action |
|---------|--------|
| `F` | Forward |
| `B` | Backward |
| `S` | Stop |
| `SPD:150` | Set speed (PWM 0-255) |
| `0`-`9` | Speed presets |
| `A` | Auto bounce mode |
| `M` | Manual mode |

NodeMCU replies with: `STATUS:F,SPD:200,LA:0,LB:0,IP:192.168.1.42`

## License

MIT
