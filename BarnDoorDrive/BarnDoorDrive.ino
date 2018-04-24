/*
 * Stepper motor driver for
 * Barn Door Equatorial Mount
 */
#define LED_PIN          13

#define DIRECTION_PIN     3
#define STEP_PIN          2
#define MOTOR_ENABLE      7
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

int fastMotorOCR1A    = 20000;
int fastMotorPrescale = 0x09;  // ps 1

int slowMotorOCR1A    = 1875;
int slowMotorPrescale = 0x0C;  // ps 256
//=======================================

// interrupt service routine 
ISR(TIMER1_COMPA_vect)
{
  digitalWrite(LED_PIN, digitalRead(LED_PIN) ^ 1);
  digitalWrite(STEP_PIN, HIGH);
  digitalWrite(STEP_PIN, LOW);
}

void stopMotorInterrupts() {
  TIMSK1 &= ~(1 << OCIE1A);
}

void startMotorInterrupts() {
  TIMSK1 |= (1 << OCIE1A);
}

void motorStart(int direction, int speed) {
  noInterrupts();
  if (speed == MOTOR_STOP) {
    Serial.println("Stopping!");
    stopMotorInterrupts();
  } else {
    Serial.println("Not stopping!");
    int dirBit = (direction == MOTOR_OUT) ? 1 : 0;
    Serial.print("direction is ");
    Serial.println(dirBit);
    digitalWrite(DIRECTION_PIN, dirBit);
    int tccr1bTemplate = TCCR1B & TCCR1B_PRESCALE_MASK;
    Serial.print("tccr1bTemplate ");
    Serial.println(tccr1bTemplate);
    if (speed == MOTOR_FAST) {
      TCCR1A = 0;
      OCR1A = fastMotorOCR1A;
      TCNT1 = 0;
      tccr1bTemplate |= fastMotorPrescale;
      TCCR1B = tccr1bTemplate;
      Serial.print("(fast) tccr1b => ");
      Serial.println(tccr1bTemplate);
    } else {
      TCCR1A = 0;
      OCR1A = slowMotorOCR1A;
      TCNT1 = 0;
      tccr1bTemplate |= slowMotorPrescale;
      TCCR1B = tccr1bTemplate;
      Serial.print("(slow) tccr1b => ");
      Serial.println(tccr1bTemplate);
    }
    startMotorInterrupts();
  }
  interrupts();
}

void modeResetPosition() {
  motorStart(MOTOR_RETURN, MOTOR_FAST);
  // await input from limit switch
  // then stop motor
  // set mode "idle"
}

void setup() {
  Serial.begin(9600);
  Serial.println("Say Something");

  pinMode(LED_PIN, OUTPUT);
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(MOTOR_ENABLE, OUTPUT);
  pinMode(STEP_SIZE_MS1_PIN, OUTPUT);
  pinMode(STEP_SIZE_MS2_PIN, OUTPUT);
  pinMode(STEP_SIZE_MS3_PIN, OUTPUT);
  digitalWrite(STEP_SIZE_MS1_PIN, LOW);
  digitalWrite(STEP_SIZE_MS2_PIN, LOW);
  digitalWrite(STEP_SIZE_MS3_PIN, LOW);

  // Set mode "resetting position"
  modeResetPosition();
}

char inData[128];
int dataCount = 0;

void loop() {
  while (Serial.available() > 0) {
    char charRead = Serial.read();
    if (charRead == 0x0A || charRead == 0x0D) {
      inData[dataCount] = 0;
      Serial.print("\nYou said ");
      Serial.print(inData);
      if (strcmp(inData, "stop") == 0) {
        Serial.println("stopping...");
        motorStart(MOTOR_OUT, MOTOR_STOP);
      } else if (strcmp(inData, "ff") == 0) {
        motorStart(MOTOR_RETURN, MOTOR_FAST);
      } else if (strcmp(inData, "sr") == 0) {
        motorStart(MOTOR_OUT, MOTOR_SLOW);
      }
      dataCount = 0;
    } else {
      if (dataCount < 127) {
        inData[dataCount++] = charRead;
      }
    }
  }
}

