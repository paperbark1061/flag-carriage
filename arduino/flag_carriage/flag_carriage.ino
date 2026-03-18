/*
 * Flag Carriage — Arduino Nano Bluetooth Motor Controller
 * ========================================================
 * Hardware:
 *   - Arduino Nano (ATmega328P)
 *   - HM-10 BLE module (recommended for iPhone)
 *     OR HC-05 Classic Bluetooth (Android only)
 *   - L298N dual H-bridge motor driver
 *   - 12V DC gear motor (100RPM, >=20kg.cm torque)
 *   - Optional: limit switches at each end of rope
 *
 * IMPORTANT — Arduino Nano vs Micro:
 *   The Nano has only ONE hardware serial port (pins 0/1),
 *   which is shared with the USB/PC connection.
 *   Bluetooth MUST use SoftwareSerial on D10 (RX) / D11 (TX).
 *   Do NOT connect BT module while uploading sketch via USB.
 *
 * Wiring:
 *   HM-10/HC-05 TX  -> Nano D10  (SoftwareSerial RX)
 *   HM-10/HC-05 RX  -> Nano D11  (SoftwareSerial TX)
 *                       * HC-05 RX needs voltage divider: 1k + 2k
 *                       * HM-10 is 3.3V — use Nano 3.3V pin for VCC
 *   L298N IN1        -> Nano D6
 *   L298N IN2        -> Nano D7
 *   L298N ENA (PWM)  -> Nano D5
 *   Limit switch A   -> Nano D2  (INPUT_PULLUP, other leg to GND)
 *   Limit switch B   -> Nano D3  (INPUT_PULLUP, other leg to GND)
 *   L298N 12V        -> 12V battery positive
 *   L298N GND        -> Common GND (battery GND + Nano GND)
 *   L298N 5V out     -> Nano 5V pin  (powers Nano from L298N reg)
 *   HM-10 VCC        -> Nano 3.3V
 *   HM-10 GND        -> Nano GND
 *
 * Bluetooth command protocol:
 *   'F'       — Forward
 *   'B'       — Backward
 *   'S'       — Stop
 *   'A'       — Auto mode (bounces between limit switches)
 *   'M'       — Manual mode
 *   'SPD:nnn' — Set speed, nnn = 0-255 PWM value
 *   '0'..'9'  — Speed presets (0=stop, 9=full speed)
 *
 * Arduino replies with: STATUS:F,SPD:200,LA:0,LB:0
 */

#include <SoftwareSerial.h>

// ── SoftwareSerial for Bluetooth ─────────────────────────────────
// D10 = BT TX -> Nano RX
// D11 = BT RX <- Nano TX
SoftwareSerial btSerial(10, 11);  // RX, TX

// ── Pin definitions ──────────────────────────────────────────────
const int PIN_IN1     = 6;   // L298N IN1 — motor direction A
const int PIN_IN2     = 7;   // L298N IN2 — motor direction B
const int PIN_ENA     = 5;   // L298N ENA — PWM speed control
const int PIN_LIMIT_A = 2;   // Limit switch — rope end A
const int PIN_LIMIT_B = 3;   // Limit switch — rope end B

// ── State ────────────────────────────────────────────────────────
enum Direction { DIR_STOP, DIR_FORWARD, DIR_BACKWARD };

Direction currentDir   = DIR_STOP;
int       currentSpeed = 200;    // PWM 0-255 (default ~78%)
bool      autoMode     = false;
bool      limitA       = false;
bool      limitB       = false;

// ── Setup ────────────────────────────────────────────────────────
void setup() {
  pinMode(PIN_IN1,     OUTPUT);
  pinMode(PIN_IN2,     OUTPUT);
  pinMode(PIN_ENA,     OUTPUT);
  pinMode(PIN_LIMIT_A, INPUT_PULLUP);
  pinMode(PIN_LIMIT_B, INPUT_PULLUP);

  Serial.begin(115200);     // USB / debug monitor
  btSerial.begin(9600);     // BT module (HM-10 and HC-05 default to 9600)

  motorStop();
  Serial.println(F("Flag Carriage Nano Ready"));
  btSerial.println(F("FLAG_CARRIAGE_READY"));
}

// ── Main loop ────────────────────────────────────────────────────
void loop() {
  // Read limit switches (LOW = triggered — INPUT_PULLUP)
  limitA = (digitalRead(PIN_LIMIT_A) == LOW);
  limitB = (digitalRead(PIN_LIMIT_B) == LOW);

  // Auto-bounce mode
  if (autoMode) {
    runAutoMode();
  }

  // Handle incoming Bluetooth commands
  if (btSerial.available()) {
    String cmd = btSerial.readStringUntil('\n');
    cmd.trim();
    handleCommand(cmd);
  }

  // Safety: stop if limit hit in manual mode
  if (!autoMode) {
    if (currentDir == DIR_FORWARD  && limitB) motorStop();
    if (currentDir == DIR_BACKWARD && limitA) motorStop();
  }
}

// ── Command handler ──────────────────────────────────────────────
void handleCommand(String cmd) {
  if (cmd.length() == 0) return;

  Serial.print(F("CMD: ")); Serial.println(cmd);

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
      Serial.println(F("Auto mode ON"));
      btSerial.println(F("MODE:AUTO"));
      break;

    case 'M':
      autoMode = false;
      motorStop();
      Serial.println(F("Manual mode"));
      btSerial.println(F("MODE:MANUAL"));
      break;

    default:
      if (cmd.startsWith("SPD:")) {
        // e.g. "SPD:180" — set PWM directly
        int spd = cmd.substring(4).toInt();
        currentSpeed = constrain(spd, 0, 255);
        analogWrite(PIN_ENA, currentSpeed);
        Serial.print(F("Speed: ")); Serial.println(currentSpeed);
        btSerial.print(F("SPD:")); btSerial.println(currentSpeed);
      } else if (c >= '0' && c <= '9') {
        // Single digit preset: 0=0%, 9=100%
        int pct = (c - '0') * 100 / 9;
        currentSpeed = map(pct, 0, 100, 0, 255);
        analogWrite(PIN_ENA, currentSpeed);
        sendStatus();
      }
      break;
  }
}

// ── Auto mode: bounce between limit switches ─────────────────────
void runAutoMode() {
  if (currentDir == DIR_STOP) {
    motorForward();
  }
  if (currentDir == DIR_FORWARD && limitB) {
    delay(200);
    motorBackward();
  }
  if (currentDir == DIR_BACKWARD && limitA) {
    delay(200);
    motorForward();
  }
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
  btSerial.println(status);
  Serial.println(status);
}
