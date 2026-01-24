// main.ino 
// Copyright 2026 Mikhail Sulim <KevinDev64>
// Email: KevinDev64 <kevindev56@yandex.ru>

// include libs
#include <OneWire.h> // Ground temp sens interface
#include <DallasTemperature.h> // Ground temp sens lib

#include <DHT.h> // Air temp and humidity sensor lib

#include <Wire.h> // I2C lib
#include <LiquidCrystal_I2C.h> // LCD lib 


// constant global values section ---begin---
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
