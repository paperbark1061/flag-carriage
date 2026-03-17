/*
 * Flag Carriage — Arduino Bluetooth Motor Controller
 * ====================================================
 * Hardware:
 *   - Arduino Micro (or Nano / Uno)
 *   - HC-05 or HM-10 Bluetooth module
 *   - L298N or L9110S motor driver
 *   - 12V DC gear motor (100RPM)
 *   - Optional: limit switches at each end of rope
 *
 * Bluetooth command protocol (single char or short string):
 *   'F'  — Forward (motor drives carriage one way)
 *   'B'  — Backward (reverse)
 *   'S'  — Stop
 *   '0'..'9' — Speed 0–100% (mapped to PWM 0–255)
 *   'A'  — Auto mode (back and forth between limit switches)
 *   'M'  — Manual mode
 *
 * Wiring:
 *   HC-05/HM-10 TX  -> Arduino RX1 (pin 0)
 *   HC-05/HM-10 RX  -> Arduino TX1 (pin 1) via voltage divider if 5V module
 *   L298N IN1       -> Arduino pin 6
 *   L298N IN2       -> Arduino pin 7
 *   L298N ENA (PWM) -> Arduino pin 5
 *   Limit switch A  -> Arduino pin 2 (INPUT_PULLUP)
 *   Limit switch B  -> Arduino pin 3 (INPUT_PULLUP)
 *   L298N 12V       -> 12V battery
 *   L298N GND       -> Common GND (battery + Arduino)
 *   L298N 5V out    -> Arduino 5V (if no separate power)
 */

// ── Pin definitions ──────────────────────────────────────────────
const int PIN_IN1       = 6;    // Motor direction A
const int PIN_IN2       = 7;    // Motor direction B
const int PIN_ENA       = 5;    // PWM speed (ENA on L298N)
const int PIN_LIMIT_A   = 2;    // Limit switch — end A
const int PIN_LIMIT_B   = 3;    // Limit switch — end B

// ── State ────────────────────────────────────────────────────────
enum Direction { DIR_STOP, DIR_FORWARD, DIR_BACKWARD };

Direction currentDir   = DIR_STOP;
int       currentSpeed = 200;     // PWM 0–255 (default ~78%)
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

  // Hardware serial for Bluetooth (RX0/TX1 on Arduino Micro)
  Serial1.begin(9600);   // HC-05 default baud; HM-10 uses 9600 too
  Serial.begin(115200);  // USB serial for debug

  motorStop();
  Serial.println("Flag Carriage Ready");
  Serial1.println("FLAG_CARRIAGE_READY");
}

// ── Main loop ────────────────────────────────────────────────────
void loop() {
  // Read limit switches (LOW = triggered, due to INPUT_PULLUP)
  limitA = (digitalRead(PIN_LIMIT_A) == LOW);
  limitB = (digitalRead(PIN_LIMIT_B) == LOW);

  // Auto-reverse mode
  if (autoMode) {
    runAutoMode();
  }

  // Handle incoming Bluetooth commands
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    cmd.trim();
    handleCommand(cmd);
  }

  // Safety: stop if limit hit while in manual mode
  if (!autoMode) {
    if (currentDir == DIR_FORWARD  && limitB) motorStop();
    if (currentDir == DIR_BACKWARD && limitA) motorStop();
  }
}

// ── Command handler ──────────────────────────────────────────────
void handleCommand(String cmd) {
  if (cmd.length() == 0) return;

  Serial.print("CMD: "); Serial.println(cmd);

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
      Serial.println("Auto mode ON");
      Serial1.println("MODE:AUTO");
      break;

    case 'M':
      autoMode = false;
      motorStop();
      Serial.println("Manual mode");
      Serial1.println("MODE:MANUAL");
      break;

    // Speed: "SPD:150" sets PWM to 150 (0-255)
    // Or single digit 0-9 maps to 0-100%
    default:
      if (cmd.startsWith("SPD:")) {
        int spd = cmd.substring(4).toInt();
        spd = constrain(spd, 0, 255);
        currentSpeed = spd;
        analogWrite(PIN_ENA, currentSpeed);
        Serial.print("Speed set: "); Serial.println(currentSpeed);
        Serial1.print("SPD:"); Serial1.println(currentSpeed);
      } else if (c >= '0' && c <= '9') {
        // Single digit 0-9 → speed percentage
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
    // Start moving forward
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

// ── Motor control helpers ────────────────────────────────────────
void motorForward() {
  currentDir = DIR_FORWARD;
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, currentSpeed);
  Serial.println(">> FORWARD");
}

void motorBackward() {
  currentDir = DIR_BACKWARD;
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, HIGH);
  analogWrite(PIN_ENA, currentSpeed);
  Serial.println(">> BACKWARD");
}

void motorStop() {
  currentDir = DIR_STOP;
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, 0);
  Serial.println(">> STOP");
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
  Serial1.println(status);
  Serial.println(status);
}
