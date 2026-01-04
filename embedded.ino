#define BLYNK_TEMPLATE_ID "TMPL6BFogGfR6"
#define BLYNK_TEMPLATE_NAME "Automated Firetruck"
#define BLYNK_AUTH_TOKEN "J4ePVy7zTh0MWif2cmXDdu4ULR8Cao5Z"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

#define IN1 25
#define IN2 26
#define IN3 27
#define IN4 14
#define ENA 32
#define ENB 33

#define FLAME_PIN 17
#define SCAN_SERVO_PIN 19
#define AIM_SERVO_PIN 18
#define PUMP_PIN 16
#define ALERT_PIN 23

unsigned long pumpStartTime = 0;
const unsigned long pumpDuration = 500;
bool pumpRunning = false;

char ssid[] = "WeLoveEmbedded";
char pass[] = "123456";

int speedValue = 200;

Servo scanServo;
Servo aimServo;

int servoPos = 20;
int servoDirection = 1;
unsigned long lastServoUpdate = 0;
const int servoDelay = 15;

bool aimLocked = false;

bool alertState = false;
bool sirenState = false;
unsigned long lastSirenToggle = 0;
const unsigned long sirenInterval = 300;

bool flameDetected = false;

int pumpMin = 40;
int pumpMax = 140;

void smoothMoveServo(Servo &servo, int startPos, int endPos, int stepDelay = 5) {
  if (startPos < endPos) {
    for (int pos = startPos; pos <= endPos; pos++) {
      servo.write(pos);
      delay(stepDelay);
    }
  } else {
    for (int pos = startPos; pos >= endPos; pos--) {
      servo.write(pos);
      delay(stepDelay);
    }
  }
}

void setSpeed(int s) {
  analogWrite(ENA, s);
  analogWrite(ENB, s);
}

void forward() {
  setSpeed(speedValue);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void backward() {
  setSpeed(speedValue);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void leftTurn() {
  setSpeed(speedValue);
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

void rightTurn() {
  setSpeed(speedValue);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}

void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

BLYNK_WRITE(V1) { if (param.asInt()) forward(); }
BLYNK_WRITE(V2) { if (param.asInt()) backward(); }
BLYNK_WRITE(V3) { if (param.asInt()) leftTurn(); }
BLYNK_WRITE(V4) { if (param.asInt()) rightTurn(); }
BLYNK_WRITE(V7) { if (param.asInt()) stopCar(); }

BLYNK_WRITE(V8) {
  speedValue = param.asInt();
  setSpeed(speedValue);
}

BLYNK_WRITE(V9) {
  if (param.asInt()) {
    digitalWrite(PUMP_PIN, LOW);
    pumpStartTime = millis();
    pumpRunning = true;
  }
}

BLYNK_WRITE(V11) {
  alertState = param.asInt();
  if (!alertState) {
    digitalWrite(ALERT_PIN, LOW);
    sirenState = false;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

  pinMode(FLAME_PIN, INPUT_PULLUP);

  pinMode(ALERT_PIN, OUTPUT);
  digitalWrite(ALERT_PIN, LOW);

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);

  scanServo.attach(SCAN_SERVO_PIN);
  scanServo.write(servoPos);

  aimServo.attach(AIM_SERVO_PIN);
  aimServo.write(90);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}

void loop() {
  Blynk.run();

  bool flameNow = (digitalRead(FLAME_PIN) == LOW);

  if (flameNow != flameDetected) {
    flameDetected = flameNow;

    if (flameDetected) {
      Blynk.virtualWrite(V10, "FIRE DETECTED");
    } else {
      Blynk.virtualWrite(V10, "NO FIRE");
    }
  }

  if (pumpRunning && millis() - pumpStartTime >= pumpDuration) {
    digitalWrite(PUMP_PIN, HIGH);
    pumpRunning = false;
  }

  if (flameDetected && !aimLocked) {
    int correctedAimAngle = map(servoPos, 20, 160, pumpMin, pumpMax);
    smoothMoveServo(aimServo, aimServo.read(), correctedAimAngle, 5);
    aimLocked = true;
  }

  if (!flameDetected && aimLocked) {
    int correctedCenter = map(90, 20, 160, pumpMin, pumpMax);
    smoothMoveServo(aimServo, aimServo.read(), correctedCenter, 5);
    aimLocked = false;
  }

  if (!flameDetected) {
    unsigned long now = millis();
    if (now - lastServoUpdate >= servoDelay) {
      servoPos += servoDirection;
      if (servoPos >= 160) servoDirection = -1;
      if (servoPos <= 20)  servoDirection = 1;

      scanServo.write(servoPos);
      lastServoUpdate = now;
    }
  }

  if (alertState) {
    unsigned long now = millis();
    if (now - lastSirenToggle >= sirenInterval) {
      sirenState = !sirenState;
      digitalWrite(ALERT_PIN, sirenState ? HIGH : LOW);
      lastSirenToggle = now;
    }
  }
}
