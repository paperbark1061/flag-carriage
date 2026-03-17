# Flag Carriage

Self-driven flag carriage that rides along a fixed rope — controlled via Bluetooth from an iPhone app.

## Hardware

| Component | Spec |
|-----------|------|
| Microcontroller | Arduino Micro |
| Bluetooth | HC-05 (classic) or HM-10 (BLE) |
| Motor driver | L298N dual H-bridge |
| Motor | 12V DC gear motor, 100RPM |
| Battery | 12V (drill battery or Li-ion pack) |
| Limit switches | 2x micro switches (optional, auto-reverse) |

## Repository structure

```
arduino/
  flag_carriage/
    flag_carriage.ino   <- Upload this to your Arduino Micro
ios/                    <- iOS Swift app (coming soon)
```

## Bluetooth command protocol

| Command | Action |
|---------|--------|
| F | Forward |
| B | Backward |
| S | Stop |
| SPD:150 | Set speed (PWM 0-255) |
| 0-9 | Speed presets (0-100%) |
| A | Auto mode (bounces between limit switches) |
| M | Manual mode |

Arduino replies with status strings e.g. STATUS:F,SPD:200,LA:0,LB:0

## Wiring summary

HC-05 TX  -> Arduino RX1 (pin 0)
HC-05 RX  -> Arduino TX1 (pin 1)
L298N IN1 -> Arduino pin 6
L298N IN2 -> Arduino pin 7
L298N ENA -> Arduino pin 5 (PWM)
Limit A   -> Arduino pin 2 (INPUT_PULLUP)
Limit B   -> Arduino pin 3 (INPUT_PULLUP)
12V batt  -> L298N 12V + GND
L298N 5V  -> Arduino 5V

## License

MIT
