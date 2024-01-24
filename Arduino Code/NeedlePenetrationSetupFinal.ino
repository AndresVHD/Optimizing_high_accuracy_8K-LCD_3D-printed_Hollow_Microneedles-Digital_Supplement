// Definitions of pin connections and constants
#define STEP_PIN 11         // Pin connected to the STEP input on the motor driver
#define DIR_PIN 10          // Pin connected to the DIR input on the motor driver
#define ENABLE_PIN 12       // Pin connected to the ENABLE input on the motor driver
#define ENDSTOP_PIN 2       // Pin connected to the endstop switch
#define LOADCELL_PIN A0     // Analog pin connected to the load cell
#define BUTTON_PIN 3        // Pin connected to the push button

#include "ADS1X15.h"
ADS1115 ADS(0x48);

#define debug 0

// Constants for motor movement
const int homingSpeed = 10; // Steps per second for homing
const int downDistance = 42; //in mm Steps for downward movement (100mm = 50 rotations)
const int stepsPerRevolution = 200; // Steps per revolution
const int pitch = 2;    // Leadscrew pitch in mm

// Calibration values for the load cell
long baseReading; // 0-punt van de sensor
float force;
int referentienulpunt = 0;
float lastForce;

bool programStarted = false; // Flag to track program start

const int metingenPerMM = 200;
const int maximaleAfstandMetingMM = 10; //in mm

// Function to move the motor a specific number of steps at a defined speed
void moveMotor(long steps, int speed) {
  digitalWrite(DIR_PIN, (steps > 0) ? LOW : HIGH); // Set direction
  for (long i = 0; i < abs(steps); ++i) {
    digitalWrite(STEP_PIN, HIGH);
    //delayMicroseconds(50);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(speed);
  }
}

// Function to read load cell values and display calibrated force
long readLoadCellADS() {
  int sensorValue = ADS.getValue(); // Read the analog value from the load cell
  float force = calibrateForceADS(sensorValue - referentienulpunt); // Calibrate the raw value to force in Newtons

  //Serial.print("Load Cell - Raw Value: ");
  //Serial.print(sensorValue);
  //Serial.print(" | Force (N): ");
  if (force > 0.10 && referentienulpunt != 0) {
    Serial.println(force, 3); // Display force with 4 decimal places
    lastForce = force;
  }
  return sensorValue;
}
// Function to calibrate raw value to force in Newtons
float calibrateForceADS(int sensorValue) {
  float force = (float) map(sensorValue, 0, 4762, 0 * 1000, 9.81 * 1000) / 1000;
  //float force = baseForce + (loadForce - baseForce) * (sensorValue - baseValue) / (loadValue - baseValue);
  return force;
}

// Function to perform homing of the motor
void performHoming() {
  if (debug) {
    Serial.println("Start homing...");
  }
  digitalWrite(DIR_PIN, HIGH); // Set direction towards endstop

  while (digitalRead(ENDSTOP_PIN) == HIGH) {
    digitalWrite(STEP_PIN, HIGH);
    //geen tussen delay nodig
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(homingSpeed);
  }
  if (debug) {
    Serial.println("Endstop triggered. Homing completed.");
    referentienulpunt = readLoadCellADS();
    Serial.print("Calibratiefout is momenteel: ");
    Serial.println(referentienulpunt);
  }
}

// Function to move the motor downward after homing
void performDownwardMovement(int Distance) {
  long stepsToMove = (long) 3200 / 2 * Distance; // Calculate steps for downward movement
  if (debug) {
    Serial.print("Moving down ");
    Serial.print(Distance);
    Serial.println(" mm...");
  }
  moveMotor(stepsToMove, 50); // Move downward at a speed of 200 microseconds
}

void perfomMeasurement() {
  // Start displaying load cell values
  if (debug) {
    Serial.println("Starting load cell readings:");
  }
  // 200 metingen per 3200 steps
  for (int j = 0; j < maximaleAfstandMetingMM; j++) { // 10 mm bewegen
    //long start = millis();
    for (int i = 0; i < metingenPerMM; i++) { // per mm in 200 metingen 1/200ste van een mm bewegen
      moveMotor((3200 / metingenPerMM), 100); // 1/200 ste van een mm aan snelheid 250/step
      // 133 proefondervindelijk bekomen door de functies hierboven en onder met millis bij te stellen.
      // 600ms is de target per mm beweging
      //readLoadCell();
      readLoadCellADS();
      if (lastForce > 0.15 && j < (maximaleAfstandMetingMM - 2)) {
        j = (maximaleAfstandMetingMM - 2);
      }
    }
    //long end = millis() - start;
    //Serial.print("dit duurde: ");
    //Serial.println(end);
  }
}

// Setup function - runs once on startup
void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);  // Enable the motor driver

  pinMode(ENDSTOP_PIN, INPUT_PULLUP); // Set endstop pin as input with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Set button pin as input with internal pull-up resistor
  Wire.begin();
  ADS.begin();
  ADS.setGain(0);
  ADS.setMode(0);//MODE_CONTINUOUS
  ADS.setDataRate(7);
  ADS.requestADC(0);
  Serial.begin(115200); // Start serial communication
  referentienulpunt = readLoadCellADS();
}

// Loop function - runs continuously after setup
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW && !programStarted) {
    programStarted = true;
    if (debug) {
      Serial.println("Program started.");
    }
  }
  if (programStarted) {
    digitalWrite(ENABLE_PIN, LOW);  // Enable the motor driver
    delay(250);
    performHoming();
    delay(250);
    performDownwardMovement(downDistance);
    perfomMeasurement();
    performHoming();
    performDownwardMovement(5);
    programStarted = false;
    delay(250);
    digitalWrite(ENABLE_PIN, HIGH);  // disable the motor driver
  } else {
    if (debug) {
      readLoadCellADS();
    }
  }
}
