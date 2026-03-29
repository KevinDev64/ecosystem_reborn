// main.ino 
// Copyright 2026 Mikhail Sulim <KevinDev64>
// Written by: KevinDev64 <kevindev56@yandex.ru>

// include libs
#include <OneWire.h> // Ground temp sens interface
#include <DallasTemperature.h> // Ground temp sens lib
#include <DHT.h> // Air temp and humidity sensor lib
#include <Wire.h> // I2C lib
#include <LiquidCrystal_I2C.h> // LCD lib 
#include <EEPROM.h> // Arduino EEPROM lib
#include <string.h> // string type


// constant global values section 
// Software version
#define ECOSYSTEM_VERSION 1.0

// Sensors pins
#define GROUND_HUM_SENSOR_PIN A0 // Port I
#define GROUND_TEMP_SENSOR_PIN 12 // Port II
#define AIR_SENSOR_PIN 11 // Port III

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

// EEPROM init values
#define EEPROM_INIT_ADDR 1023
#define EEPROM_INIT_KEY 77

// Buttons flags
bool left_button_flag, right_button_flag;
bool ok_button_flag, esc_button_flag;
bool* buttons_array[4] {&left_button_flag, &right_button_flag, &ok_button_flag, &esc_button_flag};

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


// Timers (for millis)
uint64_t screen_timer;
uint64_t buttons_timer;

// Define sensors & LCD
OneWire oneWire(GROUND_TEMP_SENSOR_PIN);
DallasTemperature ground_temp_sensor(&oneWire);
DHT air_sensor(AIR_SENSOR_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 20, 4);


// const arrays for loops
const int relays_pins[8] = {
  DAY_LIGHT_RELAY_PIN,
  NIGHT_LIGHT_RELAY_PIN,
  AIR_HEATER_RELAY_PIN,
  GROUND_HEATER_RELAY_PIN,
  WATER_RELAY_PIN,
  VENT_IN_RELAY_PIN,
  VENT_OUT_RELAY_PIN,
  UNKNOWN_RELAY_A_PIN
};

// arrays for settings 
const String settings_names_array[8] {"air_temp_min_crit", "air_temp_min", "air_temp_max", "air_temp_max_crit", "ground_temp_min", "ground_temp_max", "ground_hum_min", "ground_hum_max"};
float* settings_values_table[8] {
  &air_temp_min_crit,
  &air_temp_min,
  &air_temp_max,
  &air_temp_max_crit,
  &ground_temp_min,
  &ground_temp_max,
  &ground_hum_min,
  &ground_hum_max
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
  for (int i = 0; i <= 7; i++) {
    pinMode(relays_pins[i], OUTPUT);
    digitalWrite(relays_pins[i], LOW);
  }
  pinMode(BUTTONS_PIN, INPUT); 
  pinMode(SETUP_JUMPER, INPUT_PULLUP);

  // SETUP
  if (EEPROM.read(EEPROM_INIT_ADDR) != EEPROM_INIT_KEY) {
    // first start => setup jumper must be setted
    while (digitalRead(SETUP_JUMPER) != 0) {
      lcd.print("ecosystem v");
      lcd.print(ECOSYSTEM_VERSION);
      lcd.setCursor(0, 1);
      lcd.print("--------------------");
      lcd.setCursor(0, 2);
      lcd.print("SET UP JMP_SETUP!");
      lcd.setCursor(0, 3);
      lcd.print("waiting...");
    }

    // show info message and start editor after delay
    print_jumper_warning();

    void set_default_values();
    void setup_settings();
  } else {
    if (digitalRead(SETUP_JUMPER) != 0) {
      print_jumper_warning();
      read_settings_from_EEPROM();
    }
  }
  read_settings_from_EEPROM();
  print_welcome();

  // power on vent (air: ecosystem -> outside)
  digitalWrite(VENT_OUT_RELAY_PIN, HIGH);

  // set screen_timer
  screen_timer = millis();
}

void setup_settings() {
  // Settings menu for changing threshold values
  int setting_index = 0;
  int eeprom_address = setting_index * 4;
  screen_timer = millis();
  buttons_timer = millis();

  // LCD update timer
  if (millis() >= (screen_timer + 750)) {
    screen_timer = millis(); 
    lcd_print_setup_settings(setting_index);
  }

  // Buttons update timer
  if (millis() >= (buttons_timer + 100)) {
    buttons_timer = millis();
    get_control_buttons_values();
    if (left_button_flag) { 
      *settings_values_table[setting_index] -= 0.1; 
      eeprom_address = setting_index * 4;
      EEPROM.write(eeprom_address, *settings_values_table[setting_index]);
    }
    if (right_button_flag) {
      *settings_values_table[setting_index] += 0.1;
      eeprom_address = setting_index * 4;
      EEPROM.write(eeprom_address, *settings_values_table[setting_index]);
    } 
    if (ok_button_flag)     {
      if (setting_index == 7) { setting_index = 0; }
      else { setting_index += 1; }
    }
    if (esc_button_flag) {
      if (setting_index == 0) { setting_index = 7; }
      else { setting_index -= 1; }
    }
  }
  
}

void lcd_print_setup_settings(int setting_index) {
  lcd.clear();
  lcd.print(settings_names_array[setting_index]);
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(*settings_values_table[setting_index]);
  lcd.setCursor(0, 2);
  lcd.print("L(-0.1)  R(+0.1)");
  lcd.setCursor(0, 3);
  lcd.print("OK-next  ESC-prev");
} 

void get_control_buttons_values() {
  uint16_t rawButtonsValue = analogRead(BUTTONS_PIN);
  if (rawButtonsValue >= 0 and rawButtonsValue <= 5) {
    for (int i = 0; i <= 3; i++) {
      *buttons_array[i] = false;
    }
  }
  if (rawButtonsValue >= 200 and rawButtonsValue <= 210) {
    for (int i = 0; i <= 3; i++) {
      *buttons_array[i] = false;
    }
    esc_button_flag = true;
  }
  if (rawButtonsValue >= 405 and rawButtonsValue <= 415) {
    for (int i = 0; i <= 3; i++) {
      *buttons_array[i] = false;
    }
    ok_button_flag = true;
  }
  if (rawButtonsValue >= 609 and rawButtonsValue <= 619) {
    for (int i = 0; i <= 3; i++) {
      *buttons_array[i] = false;
    }
    right_button_flag = true;
  }
  if (rawButtonsValue >= 813 and rawButtonsValue <= 823) {
    for (int i = 0; i <= 3; i++) {
      *buttons_array[i] = false;
    }
    left_button_flag = true;
  }
}

void set_default_values() {
  EEPROM.write(0, air_temp_min_crit);
  EEPROM.write(4, air_temp_min);
  EEPROM.write(8, air_temp_max);
  EEPROM.write(12, air_temp_max_crit);
  EEPROM.write(16, ground_temp_min);
  EEPROM.write(20, ground_temp_max);
  EEPROM.write(24, ground_hum_min);
  EEPROM.write(28, ground_hum_max);

  EEPROM.write(1023, EEPROM_INIT_KEY);
}

void print_jumper_warning() {
  lcd.clear();
  lcd.print("Please remove the");
  lcd.setCursor(0, 1);
  lcd.print("JMP_SETUP and press");
  lcd.setCursor(0, 2);
  lcd.print("RESET button after");
  lcd.setCursor(0, 3);
  lcd.print("changing settings");
  delay(3000); 
}

void read_settings_from_EEPROM() {
  for (int i = 0; i <= 7; i++) {
    *settings_values_table[i] = EEPROM.read(i * 8);
  }
}

void print_welcome() {
  lcd.clear();
  lcd.print("ecosystem v");
  lcd.print(ECOSYSTEM_VERSION);
  lcd.setCursor(0, 1);
  lcd.print("--------------------");
  lcd.setCursor(0, 2);
  lcd.print("Made by KevinDev64");
  lcd.setCursor(0, 3);
  lcd.print("Have a good day!");
  delay(3000);
}

void loop() {
  lcd.clear();
  if (millis() >= (screen_timer + 750)) {
    
  }
}