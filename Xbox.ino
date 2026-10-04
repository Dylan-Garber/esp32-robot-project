#include <Bluepad32.h>

// =============================
// DRV8871 pins
// =============================
const int RIGHT_IN1_PIN = 18;
const int RIGHT_IN2_PIN = 19;
const int LEFT_IN1_PIN = 21;
const int LEFT_IN2_PIN = 22;
const int LEFT_X_IN1_PIN = 26;
const int LEFT_X_IN2_PIN = 27;

// Xbox joystick deadzone.
// Bluepad32 joystick range is approximately -511 to +512.
const int DEADZONE = 60;

// Store the connected Xbox controller
ControllerPtr xboxController = nullptr;


// =====================================================
// Motor control
// speed = -255 to +255
//
// +255 = full forward
// -255 = full reverse
//    0 = stop
// =====================================================
void setMotor(int in1Pin, int in2Pin, int speed) {

  speed = constrain(speed, -255, 255);

  if (speed > 0) {
    // Forward
    analogWrite(in2Pin, 0);
    analogWrite(in1Pin, speed);
  }
  else if (speed < 0) {
    // Reverse
    analogWrite(in1Pin, 0);
    analogWrite(in2Pin, -speed);
  }
  else {
    // Stop / coast
    analogWrite(in1Pin, 0);
    analogWrite(in2Pin, 0);
  }
}

void stopMotors() {
  setMotor(RIGHT_IN1_PIN, RIGHT_IN2_PIN, 0);
  setMotor(LEFT_IN1_PIN, LEFT_IN2_PIN, 0);
  setMotor(LEFT_X_IN1_PIN, LEFT_X_IN2_PIN, 0);
}

// Apply the same deadzone and speed scaling independently to each axis.
// The caller sets the sign so UP / RIGHT means positive/forward.
int joystickToSpeed(int command) {
  if (abs(command) < DEADZONE)
    return 0;

  int magnitude = constrain(abs(command), DEADZONE, 512);
  int pwm = map(magnitude, DEADZONE, 512, 0, 255);
  return command > 0 ? pwm : -pwm;
}


// =====================================================
// Called when controller connects
// =====================================================
void onConnectedController(ControllerPtr ctl) {

  Serial.println("Controller connected!");

  if (xboxController == nullptr) {
    xboxController = ctl;

    Serial.print("Controller: ");
    Serial.println(ctl->getModelName());
  }
}


// =====================================================
// Called when controller disconnects
// =====================================================
void onDisconnectedController(ControllerPtr ctl) {

  Serial.println("Controller disconnected!");

  if (xboxController == ctl) {
    xboxController = nullptr;

    // SAFETY: immediately stop all motors
    stopMotors();
  }
}


// =====================================================
// Read Xbox controller and control all three motors
// =====================================================
void processController() {

  if (xboxController == nullptr ||
      !xboxController->isConnected() ||
      !xboxController->isGamepad()) {
    stopMotors();
    return;
  }


  // Each joystick's Y axis controls its own motor.
  // The left joystick's X axis independently controls the third motor.
  //
  // Bluepad32:
  // approximately -511 = stick UP
  // approximately +512 = stick DOWN
  //
  int rightY = xboxController->axisRY();
  int leftY = xboxController->axisY();
  int leftX = xboxController->axisX();
  int rightSpeed = joystickToSpeed(-rightY);  // UP = forward
  int leftSpeed = joystickToSpeed(-leftY);   // UP = forward
  int leftXSpeed = joystickToSpeed(leftX);   // RIGHT = forward

  setMotor(RIGHT_IN1_PIN, RIGHT_IN2_PIN, rightSpeed);
  setMotor(LEFT_IN1_PIN, LEFT_IN2_PIN, leftSpeed);
  setMotor(LEFT_X_IN1_PIN, LEFT_X_IN2_PIN, leftXSpeed);


  // Debug information
  if (rightSpeed != 0 || leftSpeed != 0 || leftXSpeed != 0) {
    Serial.print("Right Y: ");
    Serial.print(rightY);
    Serial.print("   Right PWM: ");
    Serial.print(rightSpeed);
    Serial.print("   Left Y: ");
    Serial.print(leftY);
    Serial.print("   Left PWM: ");
    Serial.print(leftSpeed);
    Serial.print("   Left X: ");
    Serial.print(leftX);
    Serial.print("   Left X PWM: ");
    Serial.println(leftXSpeed);
  }
}


// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);

  // Motor outputs
  pinMode(RIGHT_IN1_PIN, OUTPUT);
  pinMode(RIGHT_IN2_PIN, OUTPUT);
  pinMode(LEFT_IN1_PIN, OUTPUT);
  pinMode(LEFT_IN2_PIN, OUTPUT);
  pinMode(LEFT_X_IN1_PIN, OUTPUT);
  pinMode(LEFT_X_IN2_PIN, OUTPUT);

  // Make sure all motors start OFF
  stopMotors();


  // Setup Bluepad32
  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );

  // Allow new controllers to pair
  BP32.enableNewBluetoothConnections(true);

  Serial.println();
  Serial.println("=============================");
  Serial.println("ESP32 Xbox Motor Controller");
  Serial.println("=============================");
  Serial.println("Waiting for Xbox controller...");
}


// =====================================================
// LOOP
// =====================================================
void loop() {

  // Update Bluetooth controller information
  BP32.update();


  if (xboxController != nullptr &&
      xboxController->isConnected()) {

    processController();
  }
  else {

    // Failsafe
    stopMotors();
  }


  delay(10);
}
