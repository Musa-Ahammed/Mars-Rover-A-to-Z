
/*
 * Mongol Tori Rover | V2
 * Arduino Mega 2560
 * 6 DC motors, RC transmitter, differential drive
 * PWM control, smooth acceleration, RC failsafe and kill switch
 */

// 1. Libraries and pin definitions

#include <Arduino.h>

#define ROVER_CONFIGURATION_CONFIRMED 0

const byte STEER_PIN = 2;
const byte THROTTLE_PIN = 3;
const byte KILL_PIN = 4;
const byte ARM_PIN = 6;

const int PULSE_MIN = 1000;
const int PULSE_MID = 1500;
const int PULSE_MAX = 2000;

const int VALID_MIN = 900;
const int VALID_MAX = 2100;
const int DEADBAND = 40;

const int MAX_PWM = 100;
const int RAMP_STEP = 8;

const unsigned long SIGNAL_TIMEOUT = 25000;
const unsigned long RAMP_INTERVAL = 20;
const unsigned long ARM_DELAY = 1000;
const unsigned long FRAME_TIMEOUT = 100;

const int ARM_LOW = 1200;
const int ARM_HIGH = 1800;

bool reverseSteering = false;
bool reverseThrottle = false;


// 2. Motor configuration

struct Motor {
  byte pwm;
  byte dir;
  bool left;
  bool invert;
};

// PWM, direction, side, polarity
// Motor grouping must match the actual rover wiring.

Motor motors[] = {
  {10, 15, true,  false},
  {11, 14, false, false},
  {9,  17, true,  false},
  {8,  16, false, false},
  {12, 19, true,  false},
  {13, 18, false, false}
};

const byte MOTOR_COUNT = sizeof(motors) / sizeof(motors[0]);


// 3. Control variables

bool armed = false;
bool armLowSeen = false;
bool neutralStarted = false;

int leftSpeed = 0;
int rightSpeed = 0;

unsigned long neutralTime = 0;
unsigned long lastRamp = 0;
unsigned long lastStatus = 0;


// 4. RC signal reading

bool readRC(byte pin, int &pulse) {

  unsigned long value = pulseIn(pin, HIGH, SIGNAL_TIMEOUT);

  if (value < VALID_MIN || value > VALID_MAX) {
    return false;
  }

  pulse = (int)value;
  return true;
}

int readAxis(int pulse) {

  int difference = pulse - PULSE_MID;

  if (abs(difference) <= DEADBAND) {
    return 0;
  }

  if (difference > 0) {
    return constrain(
      map(pulse, PULSE_MID + DEADBAND, PULSE_MAX, 0, 255),
      0, 255
    );
  }

  return constrain(
    map(pulse, PULSE_MIN, PULSE_MID - DEADBAND, -255, 0),
    -255, 0
  );
}


// 5. Motor control

void stopMotors() {

  for (byte i = 0; i < MOTOR_COUNT; i++) {
    analogWrite(motors[i].pwm, 0);
  }

  leftSpeed = 0;
  rightSpeed = 0;
}

void driveMotor(byte index, int speed) {

  if (motors[index].invert) {
    speed = -speed;
  }

  if (speed != 0) {
    digitalWrite(
      motors[index].dir,
      speed > 0 ? HIGH : LOW
    );
  }

  analogWrite(
    motors[index].pwm,
    constrain(abs(speed), 0, 255)
  );
}

void updateMotors() {

  for (byte i = 0; i < MOTOR_COUNT; i++) {

    int speed = motors[i].left ? leftSpeed : rightSpeed;

    driveMotor(i, speed);
  }
}


// 6. Differential steering

void calculateDrive(int throttle, int steering,
                    int &left, int &right) {

  long l = (long)throttle + steering;
  long r = (long)throttle - steering;

  long highest = max(255L, max(abs(l), abs(r)));

  left = (int)(l * MAX_PWM / highest);
  right = (int)(r * MAX_PWM / highest);
}


// 7. Smooth acceleration and direction change

int rampSpeed(int current, int target) {

  if ((current > 0 && target < 0) ||
      (current < 0 && target > 0)) {
    target = 0;
  }

  if (current < target) {
    return min(current + RAMP_STEP, target);
  }

  if (current > target) {
    return max(current - RAMP_STEP, target);
  }

  return current;
}


// 8. Kill switch and arming

bool killActive() {

  // Normally closed contact between KILL_PIN and GND.
  return digitalRead(KILL_PIN) == HIGH;
}

void disarmRover() {

  stopMotors();

  armed = false;
  armLowSeen = false;
  neutralStarted = false;
}

void checkArming(int armPulse, int throttle, int steering) {

  if (armPulse < ARM_HIGH) {

    disarmRover();

    if (armPulse <= ARM_LOW) {
      armLowSeen = true;
    }

    return;
  }

  if (armed) {
    return;
  }

  if (!armLowSeen || throttle != 0 || steering != 0) {
    neutralStarted = false;
    return;
  }

  if (!neutralStarted) {
    neutralTime = millis();
    neutralStarted = true;
  }

  if (millis() - neutralTime >= ARM_DELAY) {

    armed = true;
    armLowSeen = false;
    neutralStarted = false;
    lastRamp = millis();

    Serial.println(F("Rover armed"));
  }
}


// 9. Serial monitoring

void printStatus() {

  if (millis() - lastStatus < 500) {
    return;
  }

  lastStatus = millis();

  Serial.print(F("Armed: "));
  Serial.print(armed);

  Serial.print(F(" | Kill: "));
  Serial.print(killActive());

  Serial.print(F(" | Left: "));
  Serial.print(leftSpeed);

  Serial.print(F(" | Right: "));
  Serial.println(rightSpeed);
}


// 10. Arduino setup

void setup() {

  Serial.begin(115200);

  pinMode(STEER_PIN, INPUT);
  pinMode(THROTTLE_PIN, INPUT);
  pinMode(ARM_PIN, INPUT);
  pinMode(KILL_PIN, INPUT_PULLUP);

  for (byte i = 0; i < MOTOR_COUNT; i++) {

    digitalWrite(motors[i].pwm, LOW);
    pinMode(motors[i].pwm, OUTPUT);

    digitalWrite(motors[i].dir, LOW);
    pinMode(motors[i].dir, OUTPUT);
  }

  stopMotors();

  Serial.println(F("Mongol Tori Rover"));

#if !ROVER_CONFIGURATION_CONFIRMED
  Serial.println(F("Configuration not confirmed"));
  Serial.println(F("Motor outputs disabled"));
#endif

  Serial.println(F("Arm LOW, then HIGH with neutral sticks"));
}


// 11. Main control loop

void loop() {

#if !ROVER_CONFIGURATION_CONFIRMED
  disarmRover();
  printStatus();
  delay(10);
  return;
#endif

  if (killActive()) {
    disarmRover();
    printStatus();
    return;
  }

  int armPulse = 0;
  int throttlePulse = 0;
  int steeringPulse = 0;

  unsigned long frameStart = millis();

  if (!readRC(ARM_PIN, armPulse)) {
    disarmRover();
    printStatus();
    return;
  }

  if (armPulse < ARM_HIGH) {
    checkArming(armPulse, 0, 0);
    printStatus();
    return;
  }

  if (!readRC(THROTTLE_PIN, throttlePulse) ||
      !readRC(STEER_PIN, steeringPulse) ||
      millis() - frameStart > FRAME_TIMEOUT) {

    disarmRover();
    printStatus();
    return;
  }

  int throttle = readAxis(throttlePulse);
  int steering = readAxis(steeringPulse);

  if (reverseThrottle) {
    throttle = -throttle;
  }

  if (reverseSteering) {
    steering = -steering;
  }

  checkArming(armPulse, throttle, steering);

  if (!armed) {
    stopMotors();
    printStatus();
    return;
  }

  int targetLeft = 0;
  int targetRight = 0;

  calculateDrive(
    throttle,
    steering,
    targetLeft,
    targetRight
  );

  if (millis() - lastRamp >= RAMP_INTERVAL) {

    lastRamp = millis();

    leftSpeed = rampSpeed(leftSpeed, targetLeft);
    rightSpeed = rampSpeed(rightSpeed, targetRight);

    updateMotors();
  }

  printStatus();
}
