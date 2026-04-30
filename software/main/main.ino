// main.ino 
// Copyright 2026 Mikhail Sulim <KevinDev64>
// Written by: KevinDev64 <kevindev56@yandex.ru>

// include libs
#include <OneWire.h> // Ground temp sens interface
#include <DallasTemperature.h> // Ground temp sens lib
#include <DHT.h> // Air temp and humidity sensor lib
#include <Wire.h> // I2C lib
#include <LiquidCrystal_I2C.h> // LCD lib 
#include <iarduino_RTC.h>
#include <EEPROM.h> // Arduino EEPROM lib
#include <string.h> // string type


// constant global values section 
// Software version
#define ECOSYSTEM_VERSION 2.0

// Sensors pins
#define GROUND_HUM_SENSOR_PIN A0 // Port I
#define GROUND_TEMP_SENSOR_PIN 12 // Port II
#define AIR_SENSOR_PIN 11 // Port III

// Relays pins 
#define DAY_LIGHT_RELAY_PIN 2 // Port H
#define NIGHT_LIGHT_RELAY_PIN 3 // Port G
#define AIR_HEATER_RELAY_PIN 4 // Port F
#define GROUND_HEATER_RELAY_PIN 6 // Port D
#define WATER_RELAY_PIN 5 // Port E
#define VENT_IN_RELAY_PIN 7 // Port C
#define VENT_OUT_RELAY_PIN 8 // Port B
#define UNKNOWN_RELAY_A_PIN 9 // Port A (by default device not connected)

// Control pins
#define LEFT_BUTTON_PIN A1 
#define RIGHT_BUTTON_PIN A2
#define OK_BUTTON_PIN A3
#define ESC_BUTTON_PIN A6 
#define SETUP_JUMPER 10 // If on setup will appear

// Ground humidity sensor calibration values 
// TODO: debug script for calibration
#define GROUND_HUM_MAX 215
#define GROUND_HUM_MIN 485

// EEPROM init values
#define EEPROM_INIT_ADDR 1023
#define EEPROM_INIT_KEY 67

// Buttons flags
bool left_button_flag, right_button_flag;
bool ok_button_flag, esc_button_flag;

// Default threshold values (can be changed in setup)
float air_temp_min_crit = 17.0; // unused (TODO: remove)
float air_temp_min = 21.0;
float air_temp_max = 27.0;
float air_temp_max_crit = 29.0;

float ground_temp_min = 18.0;
float ground_temp_max = 22.5;

float ground_hum_min = 40.0;
float ground_hum_max = 70.0;


// Relay on/off flags
bool day_light_flag, night_light_flag;
bool air_heater_flag, ground_heater_flag;
bool water_pump_flag;
bool vent_in_flag, vent_out_flag;
bool air_cooler_flag;
bool unknown_device_flag; // (can be used later)

// Type of screen
unsigned short screen_type {0};

// Timers (for millis)
uint64_t screen_timer;
uint64_t buttons_timer;
uint64_t update_type_timer;

// Blink state
bool blink_state {false};

// RTC
unsigned short rtc_hours {};
unsigned short rtc_minutes {};

// instant values
float air_temp, ground_temp;
int air_humidity, ground_humidity;

// Define sensors, LCD, RTC
OneWire oneWire(GROUND_TEMP_SENSOR_PIN);
DallasTemperature ground_temp_sensor(&oneWire);
DHT air_sensor(AIR_SENSOR_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 20, 4);
iarduino_RTC time(RTC_DS3231);

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
  lcd.home();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.noAutoscroll();

  // init custom chars
  // lcd.createChar(0, drop_full_symbol);
  // lcd.createChar(1, drop_empty_symbol);
  // lcd.createChar(2, light_symbol);

  // init sensors
  air_sensor.begin();
  ground_temp_sensor.begin();
  ground_temp_sensor.setResolution(12);

  // init RTC module
  time.begin();

  // set up pin modes and off all relays
  for (int i = 0; i <= 7; i++) {
    pinMode(relays_pins[i], OUTPUT);
    digitalWrite(relays_pins[i], LOW);
  }
  pinMode(LEFT_BUTTON_PIN, INPUT);
  pinMode(RIGHT_BUTTON_PIN, INPUT);
  pinMode(OK_BUTTON_PIN, INPUT);
  pinMode(ESC_BUTTON_PIN, INPUT);
  pinMode(SETUP_JUMPER, INPUT_PULLUP);

  // set up `false` for all flags
  day_light_flag, night_light_flag = false, false;
  air_heater_flag, ground_heater_flag, air_cooler_flag = false, false, false;
  water_pump_flag = false;
  vent_in_flag = false;
  vent_out_flag = false;
  unknown_device_flag = false;

  // SETUP
  if (EEPROM.read(EEPROM_INIT_ADDR) != EEPROM_INIT_KEY) {
    // first start => setup jumper must be setted
    while (digitalRead(SETUP_JUMPER) != 0) {
      lcd.clear();
      lcd.print("ecosystem v");
      lcd.print(String(ECOSYSTEM_VERSION));
      lcd.setCursor(0, 1);
      lcd.print("--------------------");
      lcd.setCursor(0, 2);
      lcd.print("SET UP JMP_SETUP!");
      lcd.setCursor(0, 3);
      lcd.print("waiting...");
      delay(500);
    }

    // show info message and start editor after delay
    print_jumper_warning();

    set_default_values();
    setup_settings();
  } else {
    if (digitalRead(SETUP_JUMPER) == 0) {
      print_jumper_warning();
      read_settings_from_EEPROM();
      setup_settings();
    }
  }
  read_settings_from_EEPROM();
  print_welcome();

  // power on vent (air: ecosystem -> outside)
  digitalWrite(VENT_OUT_RELAY_PIN, HIGH);

  // setup timers
  screen_timer = millis();
  update_type_timer = millis();
}

void setup_settings() {
  // Settings menu for changing threshold and target values
  int setting_index = 0;
  int eeprom_address = setting_index * 4;
  screen_timer = millis();
  buttons_timer = millis();

  while (true) {
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
}

void lcd_print_setup_settings(int setting_index) {
  lcd.clear();
  lcd.print(settings_names_array[setting_index]);
  lcd.setCursor(0, 1);
  lcd.print("Value: ");
  lcd.print(String(*settings_values_table[setting_index]));
  lcd.setCursor(0, 2);
  lcd.print("L(-0.1)  R(+0.1)");
  lcd.setCursor(0, 3);
  lcd.print("OK-next  ESC-prev");
} 

void get_control_buttons_values() {
  unsigned short left_button_raw = analogRead(LEFT_BUTTON_PIN);
  unsigned short right_button_raw = analogRead(RIGHT_BUTTON_PIN);
  unsigned short ok_button_raw = analogRead(OK_BUTTON_PIN);
  unsigned short esc_button_raw = analogRead(ESC_BUTTON_PIN);

  left_button_flag = false;
  right_button_flag = false;
  ok_button_flag = false;
  esc_button_flag = false;

  if (left_button_raw > 100) {
    left_button_flag = true;
  }
  if (right_button_raw > 100) {
    right_button_flag = true;
  }
  if (ok_button_raw > 100) {
    ok_button_flag = true;
  }
  if (esc_button_raw > 100) {
    esc_button_flag = true;
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
  lcd.print("Please, remove the");
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
    *settings_values_table[i] = EEPROM.read(i * 4);
  }
}

void print_welcome() {
  randomSeed(analogRead(A7));
  int r = random(5);
  lcd.clear();
  lcd.print("ecosystem v");
  lcd.print(String(ECOSYSTEM_VERSION));
  lcd.setCursor(0, 1);
  lcd.print("--------------------");
  lcd.setCursor(0, 2);
  lcd.print("Made by KevinDev64");
  lcd.setCursor(0, 3);
  lcd.print("You define a life.");

  delay(3000);
}

void print_info_screen(uint8_t screen_type) {
  lcd.clear();
  switch (screen_type) {
    case 0:
      print_screen_0();
      break;
    case 1:
      print_screen_1();
      break;
    case 2:
      print_screen_2();
      break;
  }
}

void print_screen_0() {
  print_ecosystem_status();

  lcd.print(" ");
  lcd.print(String(rtc_hours));
  lcd.print(":");
  lcd.print(String(rtc_minutes));

  lcd.print(" ");

  lcd.setCursor(0, 1);
  print_flag_state(air_heater_flag, air_cooler_flag, String("AIR"));
  lcd.print("  ");
  print_flag_state(ground_heater_flag, false, String("GROUND"));
  lcd.print("  ");
  print_light_state();

  lcd.setCursor(0, 2);
  print_water_state();

  lcd.print("  ");
  print_vent_state();

  lcd.setCursor(0, 3);
  lcd.print("ecosystem v");
  lcd.print(ECOSYSTEM_VERSION);

  lcd.print(" p0");
}

void print_screen_1() {
  print_ecosystem_status();

  lcd.print(" ");
  lcd.print(String(rtc_hours));
  lcd.print(":");
  lcd.print(String(rtc_minutes));

  lcd.print(" ");

  lcd.setCursor(0, 1);
  print_sensors(air_temp, air_humidity, String("AIR   "));
  
  lcd.setCursor(0, 2);
  print_sensors(ground_temp, ground_humidity, String("GROUND"));

  lcd.setCursor(0, 3);
  lcd.print("ecosystem v");
  lcd.print(ECOSYSTEM_VERSION);
  lcd.print(" p1");
}

void print_screen_2() {
  print_ecosystem_status();

  lcd.print(" ");
  lcd.print(String(rtc_hours));
  lcd.print(":");
  lcd.print(String(rtc_minutes));

  lcd.print(" ");

  lcd.setCursor(0, 1);
  lcd.print("made by KevinDev64");

  lcd.setCursor(0, 2);
  lcd.print("have a good day!");

  lcd.setCursor(0, 3);
  lcd.print("ecosystem v");
  lcd.print(ECOSYSTEM_VERSION);
  lcd.print(" p2");
}

void print_sensors(float temperature, int humudity, String name) {
  lcd.print(name);
  lcd.print(" ");
  lcd.print(String(temperature));
  lcd.print(" ");
  lcd.print(String(humudity));
}

void print_vent_state() {
  lcd.print("VENT:");
  if (vent_in_flag and vent_out_flag) {
    lcd.print("A");
    return;
  }
  if (vent_in_flag) {
    lcd.print("i");
    return;
  }
  if (vent_out_flag) {
    lcd.print("o");
    return;
  }
}

void print_water_state() {
  lcd.print("WATER:");
  if (water_pump_flag) {
    lcd.print("U");
    // lcd.write(1);
  } else {
    // lcd.write(0);
    lcd.print("O");
  }
}

void print_light_state() {
  // lcd.write(2);
  lcd.print("L");
  lcd.print(":");
  if (day_light_flag) {
    lcd.print("A");
    return;
  }
  if (night_light_flag) {
    lcd.print("N");
    return;
  }
  lcd.print("x");
  return;
}

void print_flag_state(bool positive_changes_flag, bool negative_changes_flag, String name) {
  lcd.print(name);
  lcd.print(":");
  if (positive_changes_flag) {
    lcd.print("U");
    return;
  }
  if (negative_changes_flag) {
    lcd.print("D");
    return;
  }
  if ((!positive_changes_flag) and !(negative_changes_flag)) {
    lcd.print("O");
    return;
  }
}

void print_ecosystem_status() {
  lcd.print("STATUS: ");
  if (air_heater_flag or air_cooler_flag or ground_heater_flag or water_pump_flag) {
    lcd.print("bad");
  } else {
    lcd.print("ok");
  }
}

void get_rtc_time() {
  time.gettime();
  rtc_minutes = time.minutes;
  rtc_hours = time.Hours;
}

void read_air_sensor()
{
  air_temp = air_sensor.readTemperature();
  air_humidity = air_sensor.readHumidity();
}

void read_ground_sensors() {
  int raw_ground_hum;
  raw_ground_hum = analogRead(GROUND_HUM_SENSOR_PIN);
  ground_humidity = map(raw_ground_hum, GROUND_HUM_MIN, GROUND_HUM_MAX, 0, 100);

  ground_temp_sensor.requestTemperatures();
  ground_temp = ground_temp_sensor.getTempCByIndex(0);
}

void read_sensors() {
  read_air_sensor();
  read_ground_sensors();
}

// TODO: add setup of time
void check_ecosystem_state() {
  if (rtc_hours >= 0 and rtc_hours < 6)                          { 
                                                                   day_light_flag = false;
                                                                   night_light_flag = false; }
  if (rtc_hours >= 6 and rtc_hours < 7)                          {
                                                                   day_light_flag = false;
                                                                   night_light_flag = true;  }
  if (rtc_hours >= 7 and rtc_hours < 17)                         {
                                                                   day_light_flag = true;
                                                                   night_light_flag = true;  }
  if (rtc_hours >= 17 and rtc_hours < 20)                        {
                                                                   day_light_flag = false;
                                                                   night_light_flag = true;  }
  if (rtc_hours >= 20)                                           {
                                                                   day_light_flag = false;
                                                                   night_light_flag = false; }

  if (air_temp <= air_temp_min and air_cooler_flag == false)         air_heater_flag = true;
  if (air_heater_flag == true and air_temp >= air_temp_max)          air_heater_flag = false;
  if (air_temp >= air_temp_max_crit and air_heater_flag == true)     air_cooler_flag = true;
  if (air_cooler_flag == true and air_temp <= air_temp_max)          air_cooler_flag = false;
  
  if (ground_temp <= ground_temp_min)                              ground_heater_flag = true;
  if (ground_heater_flag == true and ground_temp >= ground_temp_max) ground_heater_flag = false;

  if (ground_humidity <= ground_hum_min)                                water_pump_flag = true;
  if (water_pump_flag == true and ground_humidity >= ground_hum_max)         water_pump_flag = false;
}

void apply_changes() {
  if (day_light_flag == false) { 
    digitalWrite(DAY_LIGHT_RELAY_PIN, LOW);
  }

  if (day_light_flag == true) { 
    digitalWrite(DAY_LIGHT_RELAY_PIN, HIGH);
  }

  if (night_light_flag == false) {
    digitalWrite(NIGHT_LIGHT_RELAY_PIN, LOW);
  }

  if (night_light_flag == true) { 
    digitalWrite(NIGHT_LIGHT_RELAY_PIN, HIGH);
  }

  if (air_heater_flag == false) { 
    digitalWrite(AIR_HEATER_RELAY_PIN, LOW);
  }

  if (air_heater_flag == true) { 
    digitalWrite(AIR_HEATER_RELAY_PIN, HIGH); 
  }

  if (air_heater_flag or air_cooler_flag) {
    digitalWrite(VENT_IN_RELAY_PIN, HIGH);
  }

  if ((!air_heater_flag) and (!air_cooler_flag)) {
    digitalWrite(VENT_IN_RELAY_PIN, LOW);
  }

  if (ground_heater_flag == false) { 
    digitalWrite(GROUND_HEATER_RELAY_PIN, LOW);
  }

  if (ground_heater_flag == true) { 
    digitalWrite(GROUND_HEATER_RELAY_PIN, HIGH);
  }

  if (water_pump_flag == false) { 
    digitalWrite(WATER_RELAY_PIN, LOW);
  }

  if (water_pump_flag == true) { 
    digitalWrite(WATER_RELAY_PIN, HIGH);
  }
}

void loop() {
  get_rtc_time();
  read_sensors();
  
  check_ecosystem_state();
  apply_changes();

  if (millis() >= (screen_timer + 750)) {
    screen_timer = millis();
    print_info_screen(screen_type);
  }
  
  if (millis() >= (update_type_timer + 10000)) {
    update_type_timer = millis();
    if (screen_type == 2) {
      screen_type = 0;
    } else {
      screen_type += 1;
    }
  }
}