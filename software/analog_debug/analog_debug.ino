#include <LiquidCrystal_I2C.h>
#include <string.h>

// constant global values section 
// Software version
#define ECOSYSTEM_VERSION 2.0

// pins
#define LEFT_BUTTON_PIN A1 
#define RIGHT_BUTTON_PIN A2
#define OK_BUTTON_PIN A3
#define ESC_BUTTON_PIN A6
#define GROUND_HUM_SENSOR_PIN A0 // Port I

// instant values
unsigned short leftButton, rightButton, okButton, escButton;
uint16_t rawHumudity {};

// define LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.clear();

  welcome();
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(OK_BUTTON_PIN, INPUT);
  pinMode(ESC_BUTTON_PIN, INPUT);
  pinMode(GROUND_HUM_SENSOR_PIN, INPUT);
}

void loop() {
  delay(500);
  getSensorValues();
  lcd.clear();
  lcd.print("G: ");
  lcd.print(String(rawHumudity));
  lcd.setCursor(0, 1);
  lcd.print(String(leftButton));
  lcd.print(" ");
  lcd.print(String(rightButton));
  lcd.print(" ");
  lcd.print(String(okButton));
  lcd.print(" ");
  lcd.print(String(escButton));
}

void getSensorValues() {
  leftButton = analogRead(LEFT_BUTTON_PIN);
  rightButton = analogRead(RIGHT_BUTTON_PIN);
  okButton = analogRead(OK_BUTTON_PIN);
  escButton = analogRead(ESC_BUTTON_PIN);
  rawHumudity = analogRead(GROUND_HUM_SENSOR_PIN);
}

void welcome() {
  lcd.clear();
  lcd.print("ecosystem v");
  lcd.print(String(ECOSYSTEM_VERSION));
  lcd.setCursor(0, 1);
  lcd.print("Analog Debug");
  lcd.setCursor(0, 3);
  lcd.print("####################");
  delay(5000);
}