#include <EEPROM.h>
#include <IRremote.h>

#define UP_BUTTON 2
#define DOWN_BUTTON 3
#define ON_OFF_BUTTON 4
#define SAVE_BUTTON 5

#define PIR_PIN A4
#define LED_PIN A2
#define RELAY_PIN A5

#define IR_PIN A3

#define DIGITS 3

#define IR_MINUS 0xFFE01F
#define IR_PLUS 0xFFA857
#define IR_EQ 0xFF906F
#define IR_NEXT 0xFF02FD
#define IR_PREV 0xFF22DD
#define IR_PLAY 0xFFC23D

unsigned long lastDebounceTime = 0;
int lastButtonState = LOW;
unsigned long debounceDelay = 300;

int timerDelay = 20;
unsigned long timerStartMillis = 0;
bool timerState = false;
bool ledState = false;

bool onOffState = false;
bool irState = false;

unsigned long segStartMillis = 0;
bool segState = false;
int displayDelay = 5;
byte segPins[7] = {A0, 10, 8, 7, 6, 13, 9};
byte digitPins[DIGITS] = {A1, 12, 11};

byte sevenSegments[12][7] = {  {1, 1, 1, 1, 1, 1, 0},  // = 0
                               {0, 1, 1, 0, 0, 0, 0},  // = 1
                               {1, 1, 0, 1, 1, 0, 1},  // = 2
                               {1, 1, 1, 1, 0, 0, 1},  // = 3
                               {0, 1, 1, 0, 0, 1, 1},  // = 4
                               {1, 0, 1, 1, 0, 1, 1},  // = 5
                               {1, 0, 1, 1, 1, 1, 1},  // = 6
                               {1, 1, 1, 0, 0, 0, 0},  // = 7
                               {1, 1, 1, 1, 1, 1, 1},  // = 8
                               {1, 1, 1, 1, 0, 1, 1},  // = 9
                               {0, 0, 0, 0, 0, 0, 0},   // off
                            };

IRrecv irrecv(IR_PIN);
decode_results results;
                            
void setup() {
  int savedDelay;

  // Initiallize Serial Communication
  Serial.begin(9600);
  
  // Start the receiver
  irrecv.enableIRIn();
  
  // Initiallize LED pins
  for (byte index = 0; index < 7; index++) {
    pinMode(segPins[index], OUTPUT);
  }

  // Initiallize digit pins
  for (byte index = 0; index < DIGITS; index++) {
    pinMode(digitPins[index], OUTPUT);
    digitalWrite(digitPins[index], HIGH);
  }

  // Initiallize button pins
  pinMode(UP_BUTTON, INPUT);
  pinMode(DOWN_BUTTON, INPUT);
  pinMode(ON_OFF_BUTTON, INPUT);
  pinMode(SAVE_BUTTON, INPUT);

  // Initiallize Infrared Receiver & PIR pins
  pinMode(IR_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // Initiallize pilot LED & Relay pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  // Read EEPROM data and initiallize timerDelay
  EEPROM.get(0, savedDelay);
  if (savedDelay > 0) timerDelay = savedDelay;
}

void loop() {
  int buttonState = readSwitch(UP_BUTTON);
  
  if (buttonState == HIGH) {
    // Increase timerDelay
    if (segState == true) timerDelay++;
  }

  if (readSwitch(DOWN_BUTTON) == HIGH) {
    // Reduce timerDelay
    if (segState == true) timerDelay--;
  }

  if (readSwitch(SAVE_BUTTON) == HIGH) {
    // Save timerDelay to EEPROM
    if (segState == true) EEPROM.put(0, timerDelay);
  }

  if (readSwitch(ON_OFF_BUTTON) == HIGH) {
    // Change auto/manual
    onOffState = !onOffState;
  }

  if (onOffState == true)
    digitalWrite(RELAY_PIN, LOW);
  else {
    digitalWrite(RELAY_PIN, HIGH);
    listenPIR();
  }

  if (irrecv.decode(&results)) {
    irState = true;
    Serial.println(results.value, HEX);

    if (results.value == IR_PLAY)
      onOffState = !onOffState;
    else if (results.value == IR_PLUS) {
      if (segState == true) timerDelay++;
    }
    else if (results.value == IR_MINUS) {
      if (segState == true) timerDelay--;
    }
      
    irrecv.resume(); // Receive the next value

    irState = true;
  }
  else
    irState = false;

  displaySegments();
}

void listenPIR() {
  if (digitalRead(PIR_PIN) == HIGH) {
    timerStartMillis = millis();
    timerState = true;
    ledState = true;
  }

  if (timerState == true) relay();

  if (ledState == true) led();
}

void relay() {
  digitalWrite(RELAY_PIN, LOW);
  
  if (millis() - timerStartMillis > timerDelay * 1000) {
    digitalWrite(RELAY_PIN, HIGH);
    timerState = false;
  }
}

void led() {
  digitalWrite(LED_PIN, HIGH);
  
  if (millis() - timerStartMillis > 200) {
    digitalWrite(LED_PIN, LOW);
    ledState = false;
  }
}

int readSwitch(int pin) {
  ///////////////////////////////////////////
  // Switch debouncing function
  ///////////////////////////////////////////
  int buttonState = digitalRead(pin);
  
  if (lastButtonState == LOW) {
    lastButtonState = buttonState;
    lastDebounceTime = millis();
    
    return(buttonState);
  }

  if (millis() - lastDebounceTime > debounceDelay)
    lastButtonState = LOW;

  return(LOW);
}

void offSegments() {
  ///////////////////////////////////////////
  // Turn off 3 Digit 7 Segments LED display
  ///////////////////////////////////////////
  for (byte index = 0; index < DIGITS; index++) {
    digitalWrite(digitPins[index], HIGH);
  }
}

void displaySegments() {
  if (lastButtonState == HIGH || irState == true) {
    segStartMillis = millis();
    segState = true;
  }

  if (millis() - segStartMillis > 5000)
    segState = false;

  if (segState == true)
    displaySegments(timerDelay, true);
  else
    offSegments();
}

void displaySegments(byte number, bool leadingZero) {
  ///////////////////////////////////////////
  // 3 Digit 7 Segments LED controlled function
  ///////////////////////////////////////////
  if (number > 99) {
    sevenSegmentsWrite(number/100, 0);
    sevenSegmentsWrite((number%100)/10, 1);
    sevenSegmentsWrite(number % 10, 2);
  }
  else if (number > 9) {
    if (leadingZero) {
      sevenSegmentsWrite(0, 0);
    }
    
    sevenSegmentsWrite(number/10, 1);
    sevenSegmentsWrite(number%10, 2);
  }
  else {
    if (leadingZero) {
      sevenSegmentsWrite(0, 0);
      sevenSegmentsWrite(0, 1);
    }
    
    sevenSegmentsWrite(number, 2);
  }
}

void sevenSegmentsWrite(byte number, byte digit) {
  ///////////////////////////////////////////
  // Write a digit at specified position on 7 Segments
  ///////////////////////////////////////////
  digitalWrite(digitPins[digit], LOW);
  sevenSegmentsWrite(number);
  delay(4);
  digitalWrite(digitPins[digit], HIGH);
}

void sevenSegmentsWrite(byte number) {
  ///////////////////////////////////////////
  // Write any digit on 7 Segments
  /////////////////////////////////////////// 
  for (byte segCount = 0; segCount < 7; ++segCount) {
    digitalWrite(segPins[segCount], sevenSegments[number][segCount]);
  }
}
