/*
 * Flag Carriage — ESP8266 NodeMCU WiFi Controller
 * =================================================
 * Replaces: Arduino Nano + Bluetooth module
 * Hardware:
 *   - ESP8266 NodeMCU v1.0 (or v2/v3 — all compatible)
 *   - L298N dual H-bridge motor driver
 *   - 12V DC gear motor (100RPM, >=20kg.cm)
 *   - Optional: limit switches at rope ends
 *
 * Communication: WebSocket on port 81
 *   iOS app connects to ws://[ESP_IP]:81
 *   Commands and replies are plain text strings.
 *
 * WiFi modes (set in config.h):
 *   STA — joins your router or iPhone hotspot
 *   AP  — ESP8266 creates "FlagCarriage" hotspot, phone joins it
 *
 * Wiring (NodeMCU GPIO):
 *   L298N IN1  -> D1 (GPIO5)
 *   L298N IN2  -> D2 (GPIO4)
 *   L298N ENA  -> D5 (GPIO14)  [PWM]
 *   Limit A    -> D6 (GPIO12)  [INPUT_PULLUP]
 *   Limit B    -> D7 (GPIO13)  [INPUT_PULLUP]
 *   L298N 12V  -> 12V battery +
 *   L298N GND  -> Common GND (battery + NodeMCU GND)
 *   L298N 5V   -> NodeMCU Vin pin  (or use NodeMCU USB)
 *
 * IMPORTANT — Power:
 *   NodeMCU runs on 3.3V logic but Vin accepts 5V.
 *   L298N 5V regulated output -> NodeMCU Vin works well.
 *   Do NOT connect 12V directly to NodeMCU.
 *
 * Arduino IDE setup:
 *   Board: "NodeMCU 1.0 (ESP-12E Module)"
 *   Libraries needed:
 *     - ESP8266WiFi        (built into ESP8266 board package)
 *     - WebSocketsServer   (install via Library Manager: "WebSockets" by Links2004)
 *
 * Command protocol (identical to Bluetooth version):
 *   'F'       — Forward
 *   'B'       — Backward
 *   'S'       — Stop
 *   'A'       — Auto mode (bounce between limit switches)
 *   'M'       — Manual mode
 *   'SPD:nnn' — Set speed 0-255
 *   '0'..'9'  — Speed presets
 *
 * ESP8266 replies with: STATUS:F,SPD:200,LA:0,LB:0
 */

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include "config.h"

// ── WebSocket server ─────────────────────────────────────────────
WebSocketsServer webSocket(WS_PORT);

// ── State ────────────────────────────────────────────────────────
enum Direction { DIR_STOP, DIR_FORWARD, DIR_BACKWARD };

Direction currentDir   = DIR_STOP;
int       currentSpeed = 200;     // PWM 0-255
bool      autoMode     = false;
bool      limitA       = false;
bool      limitB       = false;
int       wsClient     = -1;      // connected WebSocket client number

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Flag Carriage — ESP8266 WiFi Edition"));

  // Motor driver pins
  pinMode(PIN_IN1,     OUTPUT);
  pinMode(PIN_IN2,     OUTPUT);
  pinMode(PIN_ENA,     OUTPUT);
  pinMode(PIN_LIMIT_A, INPUT_PULLUP);
  pinMode(PIN_LIMIT_B, INPUT_PULLUP);
  motorStop();

  // WiFi
  setupWiFi();

  // WebSocket
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);
  Serial.print(F("WebSocket server started on port "));
  Serial.println(WS_PORT);
  Serial.print(F("Connect iOS app to: ws://"));
  Serial.print(getIP());
  Serial.print(F(":"));
  Serial.println(WS_PORT);
}

// ── Main loop ────────────────────────────────────────────────────
void loop() {
  webSocket.loop();

  limitA = (digitalRead(PIN_LIMIT_A) == LOW);
  limitB = (digitalRead(PIN_LIMIT_B) == LOW);

  if (autoMode) {
    runAutoMode();
  } else {
    if (currentDir == DIR_FORWARD  && limitB) motorStop();
    if (currentDir == DIR_BACKWARD && limitA) motorStop();
  }
}

// ── WiFi setup ───────────────────────────────────────────────────
void setupWiFi() {
#if WIFI_MODE == WIFI_MODE_AP
  // Create own hotspot
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println(F("AP mode — hotspot created"));
  Serial.print(F("SSID: ")); Serial.println(AP_SSID);
  Serial.print(F("IP:   ")); Serial.println(WiFi.softAPIP());

#else
  // Join existing network (router or phone hotspot)
  WiFi.mode(WIFI_STA);
  WiFi.begin(STA_SSID, STA_PASSWORD);
  Serial.print(F("Connecting to "));
  Serial.print(STA_SSID);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("Connected! IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("\nFailed to connect — falling back to AP mode"));
    // Fallback: start own hotspot so you can still reach it
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print(F("Fallback AP IP: "));
    Serial.println(WiFi.softAPIP());
  }
#endif
}

// ── Get current IP (works for both modes) ────────────────────────
String getIP() {
#if WIFI_MODE == WIFI_MODE_AP
  return WiFi.softAPIP().toString();
#else
  return (WiFi.status() == WL_CONNECTED)
    ? WiFi.localIP().toString()
    : WiFi.softAPIP().toString();
#endif
}

// ── WebSocket event handler ──────────────────────────────────────
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      wsClient = num;
      Serial.print(F("WS client connected: ")); Serial.println(num);
      webSocket.sendTXT(num, "FLAG_CARRIAGE_READY");
      sendStatus();
      break;

    case WStype_DISCONNECTED:
      Serial.print(F("WS client disconnected: ")); Serial.println(num);
      if (wsClient == num) wsClient = -1;
      motorStop();   // Safety: stop motor if app disconnects
      break;

    case WStype_TEXT: {
      String cmd = String((char*)payload);
      cmd.trim();
      Serial.print(F("CMD: ")); Serial.println(cmd);
      handleCommand(cmd, num);
      break;
    }

    default:
      break;
  }
}

// ── Command handler ──────────────────────────────────────────────
void handleCommand(String cmd, uint8_t clientNum) {
  if (cmd.length() == 0) return;
  char c = cmd.charAt(0);

  switch (c) {
    case 'F':
      autoMode = false;
      if (!limitB) motorForward();
      sendStatus();
      break;

    case 'B':
      autoMode = false;
      if (!limitA) motorBackward();
      sendStatus();
      break;

    case 'S':
      autoMode = false;
      motorStop();
      sendStatus();
      break;

    case 'A':
      autoMode = true;
      webSocket.sendTXT(clientNum, "MODE:AUTO");
      Serial.println(F("Auto mode ON"));
      break;

    case 'M':
      autoMode = false;
      motorStop();
      webSocket.sendTXT(clientNum, "MODE:MANUAL");
      Serial.println(F("Manual mode"));
      break;

    default:
      if (cmd.startsWith("SPD:")) {
        int spd = cmd.substring(4).toInt();
        currentSpeed = constrain(spd, 0, 255);
        analogWrite(PIN_ENA, currentSpeed);
        sendStatus();
      } else if (c >= '0' && c <= '9') {
        int pct = (c - '0') * 100 / 9;
        currentSpeed = map(pct, 0, 100, 0, 255);
        analogWrite(PIN_ENA, currentSpeed);
        sendStatus();
      }
      break;
  }
}

// ── Auto mode ────────────────────────────────────────────────────
void runAutoMode() {
  if (currentDir == DIR_STOP) motorForward();
  if (currentDir == DIR_FORWARD  && limitB) { delay(200); motorBackward(); }
  if (currentDir == DIR_BACKWARD && limitA) { delay(200); motorForward();  }
}

// ── Motor helpers ────────────────────────────────────────────────
void motorForward() {
  currentDir = DIR_FORWARD;
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, currentSpeed);
  Serial.println(F(">> FORWARD"));
}

void motorBackward() {
  currentDir = DIR_BACKWARD;
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, HIGH);
  analogWrite(PIN_ENA, currentSpeed);
  Serial.println(F(">> BACKWARD"));
}

void motorStop() {
  currentDir = DIR_STOP;
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, 0);
  Serial.println(F(">> STOP"));
}

// ── Status broadcast ─────────────────────────────────────────────
void sendStatus() {
  String status = "STATUS:";
  status += (currentDir == DIR_FORWARD)  ? "F" :
            (currentDir == DIR_BACKWARD) ? "B" : "S";
  status += ",SPD:";
  status += currentSpeed;
  status += ",LA:";
  status += limitA ? "1" : "0";
  status += ",LB:";
  status += limitB ? "1" : "0";
  status += ",IP:";
  status += getIP();
  if (wsClient >= 0) webSocket.sendTXT(wsClient, status);
  Serial.println(status);
}
