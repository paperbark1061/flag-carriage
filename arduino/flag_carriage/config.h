/*
 * config.h — WiFi credentials and settings
 * ==========================================
 * Edit this file with your network details before uploading.
 * Do NOT commit real passwords to a public repo.
 *
 * MODE OPTIONS:
 *   WIFI_MODE_STA  — joins your existing WiFi network (router / phone hotspot)
 *   WIFI_MODE_AP   — ESP8266 creates its own hotspot (no router needed)
 *
 * Recommendation:
 *   Use STA mode when at a fixed location with a known network.
 *   Use AP mode in the field or when no router is available.
 *   The iOS app supports both — just enter the correct IP.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ── WiFi Mode ────────────────────────────────────────────────────
// Choose ONE of:
//   WIFI_MODE_STA   — connect to existing network
//   WIFI_MODE_AP    — create own hotspot
#define WIFI_MODE WIFI_MODE_STA

// ── STA Mode settings (joining existing network) ─────────────────
#define STA_SSID     "YOUR_WIFI_NAME"
#define STA_PASSWORD "YOUR_WIFI_PASSWORD"

// ── AP Mode settings (ESP8266 creates its own hotspot) ───────────
#define AP_SSID      "FlagCarriage"
#define AP_PASSWORD  "carriage123"    // min 8 chars, or "" for open
#define AP_IP        "192.168.4.1"    // default AP gateway IP

// ── WebSocket server ─────────────────────────────────────────────
#define WS_PORT 81                    // iOS app connects to ws://[IP]:81

// ── Motor pins (NodeMCU GPIO numbers) ────────────────────────────
// NodeMCU D-pin -> GPIO mapping:
//   D1 = GPIO5   D2 = GPIO4   D5 = GPIO14
//   D6 = GPIO12  D7 = GPIO13
#define PIN_IN1     5    // D1 — L298N IN1
#define PIN_IN2     4    // D2 — L298N IN2
#define PIN_ENA     14   // D5 — L298N ENA (PWM)
#define PIN_LIMIT_A 12   // D6 — Limit switch A
#define PIN_LIMIT_B 13   // D7 — Limit switch B

#endif // CONFIG_H
