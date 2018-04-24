/*
 * Stepper motor driver for
 * Barn Door Equatorial Mount
 * 
 * TODO / CONSIDER
 *  - software end stop at 500 x 400 steps (approx 5" travel on
 *    20 tpi screw, with 5:1 step down and 400 steps / revolution)
 *  - from running, stop on first press of control button, enter
 *    rewind mode on a second press
 *  - microstep mode (on normal run?) to reduce vibration / jitter
 *  - limit run by step count
 * 
 */
#define LED_PIN          13

#define DIRECTION_PIN     3
#define STEP_PIN          2
#define MOTOR_ENABLE      7

#define STOP_PIN          8
#define CONTROL_PIN       9

#define STEP_SIZE_MS1_PIN 4
#define STEP_SIZE_MS2_PIN 5
#define STEP_SIZE_MS3_PIN 6

#define MOTOR_RETURN 0
#define MOTOR_OUT    1
#define MOTOR_STOP   0
#define MOTOR_SLOW   1
#define MOTOR_FAST   2

#define TCCR1B_PRESCALE_MASK 0xE0

//=======================================
// step speeds @ 400 steps/rev:
//         Hz      RPM
// fast = 800      120
// slow = 200/6    1/5

int fastMotorOCR1A    = 40000; // temp; should be 20000;
int fastMotorPrescale = 0x09;  // ps 1

int slowMotorOCR1A    = 1875;
int slowMotorPrescale = 0x0C;  // ps 256
//=======================================

// null routine
void idle() {}

void (* volatile isr)() = idle;
volatile int lifeCounter = 0;

// trigger the motor routine 
void stepMotor()
{
  digitalWrite(STEP_PIN, HIGH);
  digitalWrite(STEP_PIN, LOW);
}

// stepping until stop pin
void stepToStopISR() {
  stepMotor();
  if (digitalRead(STOP_PIN) == 0) {
    setModeWaitToStart();
  }
}

// wait until control pin then start
void waitToStartISR() {
  if (digitalRead(CONTROL_PIN) == 0) {
    isr = waitStartReleaseISR;
  }
}

// wait until control pin released then start
void waitStartReleaseISR() {
  if (digitalRead(CONTROL_PIN) == 1) {
    setModeRunning();
  }
}

// running until control pin
void stepToControlISR() {
  stepMotor();
  if (digitalRead(CONTROL_PIN) == 0) {
    setModeRewinding();
  }
}

// master interrupt service routine 
ISR(TIMER1_COMPA_vect)
{
  lifeCounter++;
  if (lifeCounter % 16 == 0) {
      digitalWrite(LED_PIN, digitalRead(LED_PIN) ^ 1);
  }
  // call supporting mode behavior
  (*isr)();
}

void stopMotorInterrupts() {
  TIMSK1 &= ~(1 << OCIE1A);
}

void startMotorInterrupts() {
  TIMSK1 |= (1 << OCIE1A);
}

void motorControl(int direction, int speed) {
  int dirBit = (direction == MOTOR_OUT) ? 1 : 0;
  digitalWrite(DIRECTION_PIN, dirBit);
  int tccr1bTemplate = TCCR1B & TCCR1B_PRESCALE_MASK;
  if (speed == MOTOR_FAST) {
    TCCR1A = 0;
    OCR1A = fastMotorOCR1A;
    TCNT1 = 0;
    tccr1bTemplate |= fastMotorPrescale;
    TCCR1B = tccr1bTemplate;
  } else {
    TCCR1A = 0;
    OCR1A = slowMotorOCR1A;
    TCNT1 = 0;
    tccr1bTemplate |= slowMotorPrescale;
    TCCR1B = tccr1bTemplate;
  }
}

void setModeRewinding() {
  Serial.println("Rewinding mode...");
  motorControl(MOTOR_RETURN, MOTOR_FAST);
  isr = stepToStopISR;
}

void setModeWaitToStart() {
  Serial.println("Wait for start mode...");
  motorControl(MOTOR_OUT, MOTOR_SLOW); // get ready
  isr = waitToStartISR;
}

void setModeRunning() {
  Serial.println("Normal run mode...");
  motorControl(MOTOR_OUT, MOTOR_SLOW);
  isr = stepToControlISR;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Command (stop, rew, run):");

  pinMode(LED_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);

  pinMode(STEP_SIZE_MS1_PIN, OUTPUT);
  pinMode(STEP_SIZE_MS2_PIN, OUTPUT);
  pinMode(STEP_SIZE_MS3_PIN, OUTPUT);

  pinMode(STOP_PIN, INPUT_PULLUP);
  pinMode(CONTROL_PIN, INPUT_PULLUP);

  digitalWrite(STEP_SIZE_MS1_PIN, LOW);
  digitalWrite(STEP_SIZE_MS2_PIN, LOW);
  digitalWrite(STEP_SIZE_MS3_PIN, LOW);

  setModeRewinding();
  startMotorInterrupts();
}

char inData[128];
int dataCount = 0;

void loop() {
  while (Serial.available() > 0) {
    char charRead = Serial.read();
    if (charRead == 0x0A || charRead == 0x0D) {
      inData[dataCount] = 0;
      if (strcmp(inData, "stop") == 0) {
        setModeWaitToStart();
      } else if (strcmp(inData, "rew") == 0) {
        setModeRewinding();
      } else if (strcmp(inData, "run") == 0) {
        setModeRunning();
      }
      dataCount = 0;
    } else {
      if (dataCount < 127) {
        inData[dataCount++] = charRead;
      }
    }
  }
}

