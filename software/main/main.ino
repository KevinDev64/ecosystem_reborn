// main.ino 
// Copyright 2026 Mikhail Sulim <KevinDev64>
// Email: KevinDev64 <kevindev56@yandex.ru>

// include libs
#include <OneWire.h> // Ground temp sens interface
#include <DallasTemperature.h> // Ground temp sens lib
#include <DHT.h> // Air temp and humidity sensor lib
#include <Wire.h> // I2C lib
#include <LiquidCrystal_I2C.h> // LCD lib 
#include <EEPROM.h> // Arduino EEPROM lib


// constant global values section 
// Sensors pins
#define GROUND_HUM_SENSOR_PIN A0 // Port I
#define AIR_SENSOR_PIN 11 // Port III
#define GROUND_TEMP_SENSOR_PIN 12 // Port II

// Relays pins 
#define DAY_LIGHT_RELAY_PIN 2 // Port H
#define NIGHT_LIGHT_RELAY_PIN 3 // Port G
#define AIR_HEATER_RELAY_PIN 4 // Port F
#define GROUND_HEATER_RELAY_PIN 5 // Port E
#define WATER_RELAY_PIN 6 // Port D
#define VENT_IN_RELAY_PIN 7 // Port C
#define VENT_OUT_RELAY_PIN 8 // Port B
#define UNKNOWN_RELAY_A_PIN 9 // Port A (device not connected)

// Control pins
#define BUTTONS_PIN A0 // Left, Right, OK, Cancel analog buttons
#define SETUP_JUMPER 10 // If on setup will appear


// Default threshold values (can be changed in setup)
float air_temp_min_crit = 17.0;
float air_temp_min = 23.0;
float air_temp_max = 30.0;
float air_temp_max_crit = 35.0;

float ground_temp_min = 20.0;
float ground_temp_max = 27.0;

float ground_hum_min = 30.0;
float ground_hum_max = 90.0;


// Relay on/off flags
bool day_light_flag, night_light_flag;
bool air_heater_flag, ground_heater_flag;
bool water_flag;
bool vent_in_flag, vent_out_flag;
bool unknown_device_flag; // (can be used later)


// Define sensors & LCD
OneWire oneWire(GROUND_TEMP_SENSOR_PIN);
DallasTemperature ground_temp_sensor(&oneWire);
DHT air_sensor(AIR_SENSOR_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 20, 4);


// const arrays for loops
const int relays_pins[6] = {
  DAY_LIGHT_RELAY_PIN,
  NIGHT_LIGHT_RELAY_PIN,
  AIR_HEATER_RELAY_PIN,
  GROUND_HEATER_RELAY_PIN,
  WATER_RELAY_PIN,
  VENT_IN_RELAY_PIN,
  VENT_OUT_RELAY_PIN,
  UNKNOWN_RELAY_A_PIN
};

void setup() {
  // init & clear LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.clear();

  // init sensors
  air_sensor.begin();
  ground_temp_sensor.begin();
  ground_temp_sensor.setResolution(12);

  // set up pin modes and off all relays
  for (int i = 2; i <= 9; i++) {
    pinMode(relays_pins[i], OUTPUT);
    digitalWrite(relays_pins[i], LOW);
  }
  pinMode(BUTTONS_PIN, INPUT); 
  pinMode(SETUP_JUMPER, INPUT_PULLUP);

}

void loop() {

}