#include <TM1637Display.h>
#include <Servo.h>
#include <Wire.h>
#include <RTClib.h>

// Display
#define CLK 6
#define DIO 7
TM1637Display display(CLK, DIO);

// RTC
RTC_DS3231 rtc;

// Pins
#define BUTTON_HOUR   2
#define BUTTON_MINUTE 3
#define BUTTON_MODE   4
#define BUZZER_PIN    5
#define SERVO_PIN     9

// Timekeeping
int currentHour = 0;
int currentMinute = 0;
bool alarmTriggered = false;
bool alarmOn = true;
unsigned long snoozeUntil = 0;
bool setTimeMode = false;

// Button Debouncing
unsigned long lastDebounce[3] = {0, 0, 0};
bool buttonState[3] = {HIGH, HIGH, HIGH};
bool lastButtonState[3] = {HIGH, HIGH, HIGH};
const unsigned long debounceDelay = 200;

// Servo
Servo alarmServo;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_HOUR, INPUT_PULLUP);
  pinMode(BUTTON_MINUTE, INPUT_PULLUP);
  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  alarmServo.attach(SERVO_PIN);
  alarmServo.write(0);

  display.setBrightness(0x0f);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting default time.");
    rtc.adjust(DateTime(2025, 1, 1, 0, 0, 0));
  }

  DateTime now = rtc.now();
  currentHour = now.hour();
  currentMinute = now.minute();
}

void loop() {
  unsigned long currentMillis = millis();

  if (!setTimeMode) {
    DateTime now = rtc.now();
    currentHour = now.hour();
    currentMinute = now.minute();
  }

  checkButtons();
  updateDisplay();
  checkAlarm(currentMillis);
}

void checkButtons() {
  int buttonPins[3] = {BUTTON_HOUR, BUTTON_MINUTE, BUTTON_MODE};

  for (int i = 0; i < 3; i++) {
    bool reading = digitalRead(buttonPins[i]);

    if (reading != lastButtonState[i]) {
      lastDebounce[i] = millis();
    }

    if ((millis() - lastDebounce[i]) > debounceDelay) {
      if (reading != buttonState[i]) {
        buttonState[i] = reading;

        if (buttonState[i] == LOW) {
          // Button was just pressed
          switch (i) {
            case 0: // HOUR
              if (setTimeMode) {
                currentHour = (currentHour % 12) + 1;
                Serial.print("Hour set to: ");
                Serial.println(currentHour);
              }
              break;

            case 1: // MINUTE
              if (setTimeMode) {
                currentMinute = (currentMinute + 1) % 60;
                Serial.print("Minute set to: ");
                Serial.println(currentMinute);
              }
              break;

            case 2: // MODE
              setTimeMode = !setTimeMode;
              Serial.println(setTimeMode ? "Set Time Mode ON" : "Set Time Mode OFF");

              if (!setTimeMode) {
                // Save new time to RTC with current date
                DateTime now = rtc.now();
                rtc.adjust(DateTime(now.year(), now.month(), now.day(), currentHour % 24, currentMinute, 0));
                Serial.println("Time saved to RTC.");
              }
              break;
          }
        }
      }
    }
    lastButtonState[i] = reading;
  }
}

void updateDisplay() {
  int hourToShow = currentHour % 12;
  if (hourToShow == 0) hourToShow = 12;

  int displayTime = hourToShow * 100 + currentMinute;
  display.showNumberDecEx(displayTime, 0b01000000, true); // HH:MM
}

void checkAlarm(unsigned long currentMillis) {
  if (!alarmOn || alarmTriggered || currentMillis < snoozeUntil) return;

  DateTime now = rtc.now();
  if (now.hour() == currentHour && now.minute() == currentMinute && now.second() == 0) {
    triggerAlarm();
  }
}

void triggerAlarm() {
  digitalWrite(BUZZER_PIN, HIGH);
  alarmServo.write(90);
  alarmTriggered = true;
  Serial.println("Alarm Triggered!");
}

void stopAlarm() {
  digitalWrite(BUZZER_PIN, LOW);
  alarmServo.write(0);
  alarmTriggered = false;
}




