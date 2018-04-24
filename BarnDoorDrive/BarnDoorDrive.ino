/*
 * Stepper motor driver for
 * Barn Door Equatorial Mount
 */

#define DIRECTION_PIN   ??
#define STEP_PIN        ??
#define STEP_SIZE_0_PIN ??
#define STEP_SIZE_1_PIN ??
#define STEP_SIZE_2_PIN ??

#define MOTOR_RETURN 0
#define MOTOR_OUT    1
#define MOTOR_SLOW   1
#define MOTOR_FAST   2

#define TCCR1B_PRESCALE_MASK = 0x07

//=======================================
// step speeds @ 400 steps/rev:
//         Hz      RPM
// fast = 800      120
// slow = 200/6    1/5

int fastMotorOCR1A = 20000;
int fastMotorPrescale = 0x09; // ps 1

int slowMotorOCR1A = 1875
int slowMotorPrescale = 0x0C; // ps 256
//=======================================

void stopMotorInterrupts() {
  TIMSK1 &= ~(1 << OCIE1A);
}

void startMotorInterrupts() {
  TIMSK1 |= (1 << OCIE1A);
}

void motorStart(int direction, int speed) {
  if (speed == MOTOR_STOP) {
    stopMotorInterrupts();
  } else {
    digitalWrite(DIRECTION_PIN,
      (direction == MOTOR_OUT) ? 1 : 0);
    int tccr1bTemplate = TCCR1B & TCCR1B_PRESCALE_MASK;
    
    if (speed == MOTOR_FAST) {
      OCR1A = fastMotorOCR1A;
      TCCR1B = tccr1bTemplate | fastMotorPrescale;
    } else {
      OCR1A = slowtMotorOCR1A;
      TCCR1B = tccr1bTemplate | slowMotorPrescale;
    }
    startMotorInterrupts();
  }
}

void modeResetPosition() {
  motorStart(MOTOR_RETURN, MOTOR_FAST);
  // await input from limit switch
  // then stop motor
  // set mode "idle"
}

void setup() {
  Serial.begin(9600);

  // Set mode "resetting position"
  modeResetPosition();
}

void loop() {

}
