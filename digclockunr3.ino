#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <Servo.h>

// Pins
const int CLK = 6;  
const int DIO = 7; 
const int servoPin = 8;
const int buzzerPin = 5;  // Buzzer connected to D5
const int setAlarmButton = 13;
const int incrementButton = A0;
const int decrementButton = A1;

// Objects
TM1637Display display(CLK, DIO);
RTC_DS3231 rtc;
Servo myServo;

// Alarm variables
int alarmHour = 0;
int alarmMinute = 0;
bool alarmSet = false;
bool settingAlarm = false;
unsigned long lastInteraction = 0;
const unsigned long settingTimeout = 5000;

// Debounce variables
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);

  // Init display and RTC
  display.setBrightness(2);
  myServo.attach(servoPin);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Button pins
  pinMode(setAlarmButton, INPUT_PULLUP);
  pinMode(incrementButton, INPUT_PULLUP);
  pinMode(decrementButton, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
}

void loop() {
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();

  handleButtons();

  // Check alarm
  if (alarmSet && currentHour == alarmHour && currentMinute == alarmMinute) {
    triggerAlarm();
    alarmSet = false;
  }

  // Display time (unless setting alarm)
  if (!settingAlarm) {
    int displayValue = currentHour * 100 + currentMinute;
    display.showNumberDec(displayValue, true);
  }

  // Timeout alarm setting mode
  if (settingAlarm && (millis() - lastInteraction > settingTimeout)) {
    settingAlarm = false;
    alarmSet = true;
    Serial.println("Alarm set.");
  }

  delay(200);
}

void handleButtons() {
  if (digitalRead(setAlarmButton) == LOW && millis() - lastDebounce > debounceDelay) {
    DateTime now = rtc.now();
    alarmHour = now.hour();       // Set initial alarm time to current time
    alarmMinute = now.minute();
    settingAlarm = !settingAlarm;
    lastInteraction = millis();
    Serial.println(settingAlarm ? "Entering alarm set mode..." : "Exiting alarm set mode.");
    while (digitalRead(setAlarmButton) == LOW); // wait for release
    lastDebounce = millis();
  }

  if (settingAlarm) {
    if (digitalRead(incrementButton) == LOW && millis() - lastDebounce > debounceDelay) {
      alarmMinute++;
      if (alarmMinute >= 60) {
        alarmMinute = 0;
        alarmHour = (alarmHour + 1) % 24;
      }
      showAlarmTime();
      lastInteraction = millis();
      while (digitalRead(incrementButton) == LOW);
      lastDebounce = millis();
    }

    if (digitalRead(decrementButton) == LOW && millis() - lastDebounce > debounceDelay) {
      alarmMinute--;
      if (alarmMinute < 0) {
        alarmMinute = 59;
        alarmHour = (alarmHour == 0) ? 23 : alarmHour - 1;
      }
      showAlarmTime();
      lastInteraction = millis();
      while (digitalRead(decrementButton) == LOW);
      lastDebounce = millis();
    }
  }
}

void showAlarmTime() {
  int displayValue = alarmHour * 100 + alarmMinute;
  display.showNumberDec(displayValue, true);
  Serial.print("Alarm Time Set: ");
  Serial.print(alarmHour);
  Serial.print(":");
  Serial.println(alarmMinute);
}

void triggerAlarm() {
  Serial.println("ALARM!");

  for (int i = 0; i < 5; i++) {
    tone(buzzerPin, 1000); // Start buzzer
    display.clear();
    delay(250);
    display.showNumberDec(alarmHour * 100 + alarmMinute, true);
    delay(250);
  }

  noTone(buzzerPin); // Stop buzzer

  myServo.write(90);
  delay(1000);
  myServo.write(0);
}

