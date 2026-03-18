# Flag Carriage — iOS App

SwiftUI app for controlling the flag carriage over WiFi (WebSocket).

## Requirements

- Xcode 15+
- iOS 16+
- No external Swift packages needed

## Setup

1. Open Xcode, create a new **App** project
   - Product Name: `FlagCarriage`
   - Interface: SwiftUI
   - Language: Swift
   - Minimum deployment: iOS 16

2. Delete the default `ContentView.swift` and `[AppName]App.swift`

3. Add all `.swift` files from this folder into the project (drag into Xcode navigator)

4. Build and run on your iPhone (or simulator for UI testing)

## App structure

```
FlagCarriageApp.swift       Entry point
ContentView.swift           Tab bar shell

Managers/
  ConnectionManager.swift   WebSocket connection + send/receive
  ProgramStore.swift        Data models + persistence (UserDefaults)
  RunEngine.swift           Executes programs + cattle sim logic

Views/
  ConnectView.swift         WiFi connection screen
  ManualView.swift          Hold-to-drive manual control
  ProgramView.swift         Build, save, run programs + sets
  AutoView.swift            Cattle Sim (random) + Training Sets
  ProfileEditorView.swift   Edit cattle behaviour profiles
```

## Tabs

| Tab | What it does |
|-----|--------------|
| Manual | Large hold-to-drive buttons. Speed slider + Creep/Trot/Bolt presets. Designed for one-handed mounted use. |
| Program | Build scripted runs (step sequences). Combine into Training Sets. Play back with live progress bar. |
| Cattle Sim | Random mode with cattle profiles (Lazy/Medium/Hot). Configurable speed, pause, direction change behaviour. Training Set sequential playback. |
| Connect | WiFi connection. Supports AP mode, STA mode, iPhone hotspot. Live carriage status. |

## Cattle profiles

Three built-in profiles, all fully editable:

| Profile | Aggression | Speed | Pause chance | Change freq |
|---------|-----------|-------|-------------|-------------|
| Lazy Cow | Low | 24–51% | 40% | 3.0s |
| Medium Cow | Medium | 39–75% | 25% | 1.8s |
| Hot Cow | High | 63–100% | 10% | 0.9s |

Create custom profiles for specific cattle personalities or training progressions.

## WebSocket protocol

Connects to `ws://[ESP_IP]:81`

| App sends | ESP does |
|-----------|----------|
| `F` | Forward |
| `B` | Backward |
| `S` | Stop |
| `SPD:200` | Set speed (0–255) |
| `A` | Auto bounce mode |
| `M` | Manual mode |

ESP replies: `STATUS:F,SPD:200,LA:0,LB:1,IP:192.168.4.1`
