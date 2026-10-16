#pragma once
// Pins: check against your board's pinout before wiring.
constexpr int PIN_STEER = 18;
constexpr int PIN_ESC   = 19;
constexpr int PIN_ESTOP = 27;  // normally-closed switch to GND; open or broken wire = stop

// Steering limits (us). Narrow until the real end stops are found.
constexpr int STEER_MIN_US    = 1000;
constexpr int STEER_MAX_US    = 2000;
constexpr int STEER_CENTER_US = 1500;
constexpr int STEER_SWEEP_STEP_US = 5;

// Throttle limits (us). 1500 = stop if the ESC is bidirectional; 1300 caps first tests.
constexpr int ESC_STOP_US = 1000;
constexpr int ESC_MAX_US  = 1300;
constexpr int RAMP_US_PER_TICK = 5;

constexpr unsigned long TIMEOUT_MS = 500;     // signal loss -> neutral
constexpr unsigned long TICK_MS    = 20;      // 50 Hz control loop
constexpr unsigned long ARM_MS     = 3000;    // ESC arming at minimum throttle
constexpr int WDT_TIMEOUT_S = 2;
