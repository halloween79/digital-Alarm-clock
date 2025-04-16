#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <Servo.h>

// Define the TM1637 display pins
const int CLK = 6;  
const int DIO = 7;  


TM1637Display display(CLK, DIO);

// Define the servo pin
const int servoPin = 8; 
Servo myServo;

// Define button pins
const int setTimeButtonPin = 13;
const int incrementButtonPin = A0;
const int decrementButtonPin = A1;

// Debounce delay (milliseconds)
const int debounceDelay = 50;

// Time setting mode flag
bool settingTime = false;

// Alarm time variables
int alarmHour = 7;  // Default alarm hour
int alarmMinute = 0; // Default alarm minute
bool alarmSet = false;

RTC_DS3231 rtc;

// Variables for non-blocking alarm indication
unsigned long alarmStartTime = 0;
bool alarmActive = false;

static int setTimeButtonState = 0; // Keep track of button state
static int alarmMode = 0; // 0: Set Time, 1: Set Alarm, 2: Alarm Enabled
static unsigned long lastButtonPressTime = 0; // Time of last increment/decrement press
static const unsigned long timeSettingTimeout = 5000; // 5 seconds

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize the RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // Set the time if the RTC is not running (uncomment the following lines to set the time)
  // rtc.adjust(DateTime(2023, 10, 27, 12, 34, 56)); // Year, Month, Day, Hour, Minute, Second

  // Set the pin modes for the button pins
  pinMode(setTimeButtonPin, INPUT_PULLUP);
  pinMode(incrementButtonPin, INPUT_PULLUP);
  pinMode(decrementButtonPin, INPUT_PULLUP);

  // Initialize the TM1637 display
  display.setBrightness(2); 

  // Attach the servo
  myServo.attach(servoPin);

}

// Function declaration (prototype) - IMPORTANT
void triggerAlarm(int hours, int minutes);

void loop() {
  // Get the current time from the RTC
  DateTime now = rtc.now();

  // Extract the hours, minutes, and seconds
  int hours = now.hour();
  int minutes = now.minute();
  //int seconds = now.second();

  // Check for button presses
  checkButtons(hours, minutes);

  // Format the time for display (HH:MM)
  int displayValue = hours * 100 + minutes; // Combine hours and minutes for display

  // Check if the alarm should be triggered
  if (alarmSet && hours == alarmHour && minutes == alarmMinute && !alarmActive) {
    alarmActive = true;
    alarmStartTime = millis();
    triggerAlarm(hours, minutes); // Pass hours and minutes to triggerAlarm
  }

  // Check if the alarm indication time has elapsed
  if (alarmActive && (millis() - alarmStartTime) >= 5000) {
    alarmActive = false;
    alarmSet = false; // Disable the alarm after indication
  }

  // Display the time on the TM1637 display (only if the alarm is not active)
  if (!alarmActive) {
    display.showNumberDec(displayValue, true); // Show with leading zeros
  }


  // Wait for a short period
  delay(10);
}

// Function to check for button presses and handle time setting
void checkButtons(int &hours, int &minutes) {
  static int setTimeButtonState = 0; // Keep track of button state
  static int alarmMode = 0; // 0: Set Time, 1: Set Alarm, 2: Alarm Enabled
  static unsigned long lastButtonPressTime = 0; // Time of last increment/decrement press
  static const unsigned long timeSettingTimeout = 5000; // 5 seconds

  // Set Time Button
  if (digitalRead(setTimeButtonPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(setTimeButtonPin) == LOW) {
      alarmMode = (alarmMode + 1) % 3; // Cycle through modes
      Serial.print("Alarm Mode: "); Serial.println(alarmMode); // Debug

      switch (alarmMode) {
        case 0: // Setting Current Time
          settingTime = true;
          alarmSet = false;
          break;
        case 1: // Setting Alarm Time (Alarm Disabled)
          settingTime = false;
          alarmSet = false;
          break;
        case 2: // Alarm Enabled
          settingTime = false;
          alarmSet = true;
          break;
      }
      lastButtonPressTime = millis(); // Reset timeout on mode change
      while (digitalRead(setTimeButtonPin) == LOW); // Wait for release
    }
  }

  // Increment Button
  if (digitalRead(incrementButtonPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(incrementButtonPin) == LOW) {
      if (settingTime) {
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours++;
          if (hours >= 24) {
            hours = 0;
          }
        }
        rtc.adjust(DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), hours, minutes, 0)); // Update RTC
        lastButtonPressTime = millis(); // Reset timeout
      } else {
        // Increment alarm time
        alarmMinute++;
        if (alarmMinute >= 60) {
          alarmMinute = 0;
          alarmHour++;
          if (alarmHour >= 24) {
            alarmHour = 0;
          }
        }
        //alarmSet = true; // Enable the alarm - REMOVED
      }
      while (digitalRead(incrementButtonPin) == LOW); // Wait for release
    }
  }

  // Decrement Button
  if (digitalRead(decrementButtonPin) == LOW) {
    delay(debounceDelay);
    if (digitalRead(decrementButtonPin) == LOW) {
      if (settingTime) {
        minutes--;
        if (minutes < 0) {
          minutes = 59;
          hours--;
          if (hours < 0) {
            hours = 23;
          }
        }
        rtc.adjust(DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), hours, minutes, 0)); // Update RTC
        lastButtonPressTime = millis(); // Reset timeout
      } else {
        // Decrement alarm time
        alarmMinute--;
        if (alarmMinute < 0) {
          alarmMinute = 59;
          hours--;
          if (hours < 0) {
            hours = 23;
          }
        }
        
      }
      while (digitalRead(decrementButtonPin) == LOW); // Wait for release
    }
  }

  // Check for timeout and exit setting mode
  if (settingTime && (millis() - lastButtonPressTime > timeSettingTimeout)) {
    settingTime = false; // Exit setting mode
    alarmMode = 1; // Go to alarm setting mode
    Serial.println("Timeout: Exiting time setting mode.");
  }
}

// Function to trigger the alarm 
void triggerAlarm(int hours, int minutes) { // Receive hours and minutes
  Serial.println("ALARM!");
  
  for (int i = 0; i < 5; i++) { // Blink 5 times
    display.clear(); // Turn off the display
    delay(250);
    int displayValue = hours * 100 + minutes;
    display.showNumberDec(displayValue, true); // Show the time
    delay(250);
  }

  myServo.write(90); // Move to 90 degrees
  delay(1000);
  myServo.write(0);  // Move back to 0 degrees
}
