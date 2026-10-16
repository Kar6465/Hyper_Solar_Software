#include <Arduino.h>
#include <ESP32Servo.h>
#include <esp_task_wdt.h>
#include "config.h"

// ---- Output layer: only this section writes to the servo/ESC ----
Servo steer, esc;
int steerUs = STEER_CENTER_US;
int targetUs = ESC_STOP_US;
int currentUs = ESC_STOP_US;

// ---- Input layer: serial now; swap for BLE/RC on Oct 23 ----
// Serial commands: "s1500" steering, "t1100" throttle, "w" toggle steering sweep.
// Each input source just calls setSteer()/setThrottle()/feedInput().
unsigned long lastInputMs = 0;
bool sweeping = false;
int sweepDir = 1;

void feedInput() { lastInputMs = millis(); }
void setSteer(int us) { steerUs = constrain(us, STEER_MIN_US, STEER_MAX_US); }
void setThrottle(int us) { targetUs = constrain(us, ESC_STOP_US, ESC_MAX_US); }

void handleLine(const char *line) {
  char c = line[0];
  int v = atoi(line + 1);
  if (c == 's') { sweeping = false; setSteer(v); feedInput(); }
  else if (c == 't') { setThrottle(v); feedInput(); }
  else if (c == 'w') { sweeping = !sweeping; Serial.println(sweeping ? "sweep on" : "sweep off"); }
}

void pollSerial() {  // non-blocking line reader
  static char buf[16];
  static uint8_t n = 0;
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (n) { buf[n] = 0; handleLine(buf); n = 0; }
    } else if (n < sizeof(buf) - 1) {
      buf[n++] = ch;
    }
  }
}

// ---- Safety layer: independent of input source ----
bool armed() { return millis() >= ARM_MS; }

void applySafety() {
  bool estop = digitalRead(PIN_ESTOP) == HIGH;
  bool stale = millis() - lastInputMs > TIMEOUT_MS;
  if (estop || stale || !armed()) targetUs = ESC_STOP_US;
}

void controlTick() {
  if (sweeping) {
    steerUs += sweepDir * STEER_SWEEP_STEP_US;
    if (steerUs >= STEER_MAX_US) { steerUs = STEER_MAX_US; sweepDir = -1; }
    if (steerUs <= STEER_MIN_US) { steerUs = STEER_MIN_US; sweepDir = 1; }
  }
  // Speeding up is ramped; slowing down (brake/neutral) is immediate.
  if (currentUs < targetUs) currentUs = min(currentUs + RAMP_US_PER_TICK, targetUs);
  else currentUs = targetUs;
  esc.writeMicroseconds(currentUs);
  steer.writeMicroseconds(constrain(steerUs, STEER_MIN_US, STEER_MAX_US));
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_ESTOP, INPUT_PULLUP);
  steer.setPeriodHertz(50);
  esc.setPeriodHertz(50);
  steer.attach(PIN_STEER, 500, 2500);
  esc.attach(PIN_ESC, 1000, 2000);
  steer.writeMicroseconds(steerUs);
  esc.writeMicroseconds(ESC_STOP_US);  // hold minimum throttle while the ESC arms
  lastInputMs = millis();
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  esp_task_wdt_add(NULL);
  Serial.println("ready (arming 3s)");
}

void loop() {
  static unsigned long lastTickMs = 0;
  esp_task_wdt_reset();
  pollSerial();
  applySafety();
  if (millis() - lastTickMs >= TICK_MS) {
    lastTickMs = millis();
    controlTick();
  }
}
