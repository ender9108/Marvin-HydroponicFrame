/**
 * Hydroponic frame
 * 
 * @version 1.0.0
 * @author Alexandre Berthelot
 * @since 20190404
 */

 /**
  * @todo
  * Menus
  *   - "Set light duration",
  *       - change duration in hour
  *   - "Set light time start",
  *       - change time to start light (default 8:00)
  *   - "Set bubler duration",
  *       - change duration in minute
  *   - "Set bubler intervale"
  *       - change intervale to start bubler
  */

#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int PIN_RELAY_PUMP = 4;
const int PIN_RELAY_STRIPLED = 5;
const int PIN_RELAY_BUBLER = 6;
const int PIN_LED_R  = 15;
const int PIN_LED_G  = 16;
const int PIN_LED_B  = 17;
const int PIN_PHOTO_CELL = 23;
const int PIN_PH_SENSOR = 24;
const int PIN_WATER_LEVEL_HIGH = 25;
const int PIN_WATER_LEVEL_MIDDLE = 26;
const int PIN_WATER_LEVEL_LOW = 27;
const int PIN_BTN_MENU = 11;
const int PIN_BTN_SELECT = 12;

RTC_DS3231 rtc;
Adafruit_SSD1306 display(128, 32, &Wire);
int phSensorReading;
unsigned int photocellReading;
unsigned int waterLevelReading;
unsigned long startAtmTime;
unsigned long startBublerTime = NULL;
unsigned long startMenusDisplay = NULL;
unsigned int lightHourDuration = 18; // hour
unsigned int lightMinuteDuration = 0;
unsigned int lightStartHour = 8;
unsigned int lightStartMinute = 0;
unsigned int delayStartBublerIntervale = 3600000; // milliseconde
unsigned int bublerDuration = 300000; // milliseconde
unsigned int menusDisplayDuration = 5000; // milliseconde
bool lightIsOn = false;
bool bublerIsOn = false;
bool menuIsDisplay = false;
char menus[4][26] = {
  "Set light duration",
  "Set light time start",
  "Set bubler duration",
  "Set bubler intervale"
};

void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_PHOTO_CELL, INPUT);
  pinMode(PIN_PH_SENSOR, INPUT);
  pinMode(PIN_BTN_MENU, INPUT);
  pinMode(PIN_BTN_SELECT, INPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_STRIPLED, OUTPUT);
  pinMode(PIN_RELAY_BUBLER, OUTPUT);
  pinMode(PIN_WATER_LEVEL_LOW, INPUT);
  pinMode(PIN_WATER_LEVEL_MIDDLE, INPUT);
  pinMode(PIN_WATER_LEVEL_HIGH, INPUT);

  // Set rgb led color (blue = start in progress)
  changeRgbColor(0, 0, 255);

  // Init oled
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Init oled parameters
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);

  // Display starting text
  display.println("Start in progress...");
  display.display();
  delay(1000);

  // Init RTC module (DS3231)
  if(!rtc.begin()) {
    Serial.println("Couldn't find RTC module");
    while (1);
  }

  if(rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  display.clearDisplay();
  startAtmTime = millis();
}

void loop() {
  DateTime now = rtc.now();
  unsigned long currentAtmTime = millis();
  
  photocellReading = analogRead(PIN_PHOTO_CELL);
  delay(100);
  waterLevelReading = readWaterLevelSensor();
  changeLedStatusByWaterLevel(waterLevelReading);
  delay(100);
  phSensorReading = analogRead(PIN_PH_SENSOR);
  delay(100);

  if(currentAtmTime >= (startAtmTime + delayStartBublerIntervale) && false == bublerIsOn) {
    digitalWrite(PIN_RELAY_BUBLER, HIGH);
    startBublerTime = currentAtmTime;
    bublerIsOn = true;
    Serial.println("Bubler start !");
  }

  if(true == bublerIsOn && currentAtmTime > (startBublerTime + bublerDuration)) {
    digitalWrite(PIN_RELAY_BUBLER, LOW);
    bublerIsOn = false;
    startAtmTime = currentAtmTime;
    Serial.println("Bubler shutdown !");
  }

  // Manage display menus
  menuManager(currentAtmTime);

  // Manage light
  lightManager(now);

  if(false == menuIsDisplay) {
    // Display informations on screen
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(getDateToDisplay(now));
    display.println("Luminosite : " + getLightLevelName(photocellReading));
    display.println("Niveau eau : " + getWaterLevelName(waterLevelReading));
    display.println("PH : " + String(getPhLevel(phSensorReading)));
  
    if(true == bublerIsOn) {
      display.println("Buller : actif");
    } else {
      display.println("Buller : inactif");
    }
  }
  
  display.display();
  delay(100);
}

/**
 * Display menu on screen
 * 
 * @return void
 */
void menuManager(unsigned long currentAtmTime) {
  if(digitalRead(PIN_BTN_MENU) == HIGH) {
    display.clearDisplay();
    display.setCursor(0,0);
    menuIsDisplay = true;
    startMenusDisplay = currentAtmTime;

    for(int i = 0 ; i < sizeof(menus) ; i++) {
      display.println(menus[i]);
    }
  }

  // After "menusDisplayDuration" seconde inactivity hidden menu
  if(true == menuIsDisplay && currentAtmTime > (startMenusDisplay + menusDisplayDuration)) {
    menuIsDisplay = false;
  }
}

/**
 * Light manager
 * 
 * @param DateTime now
 * @return void
 */
void lightManager(DateTime now) {
  DateTime lightStartDatetime(now.year(), now.month(), now.day(), lightStartHour, lightStartMinute, now.second());
  
  if(lightIsOn == false && now.unixtime() == lightStartDatetime.unixtime()) {
    digitalWrite(PIN_RELAY_STRIPLED, HIGH);
  }

  DateTime lightEndDateTime(lightStartDatetime + TimeSpan(lightStartDatetime.day(), lightHourDuration, lightMinuteDuration, lightStartDatetime.second()));

  if(lightIsOn == true && now.unixtime() == lightEndDateTime.unixtime()) {
    digitalWrite(PIN_RELAY_STRIPLED, LOW);
  }
}

/**
 * Get light level name (noir, sombre, lumineux, très lumineux)
 * 
 * @param integer photocellReading
 * @return String
 */
String getLightLevelName(int photocellReading) {
  String lightLevelName;
  
  if (photocellReading < 10) {
    lightLevelName = "black";
  } else if (photocellReading < 200) {
    lightLevelName = "dark";
  } else if (photocellReading < 800) {
    lightLevelName = "luminous";
  }

  return lightLevelName;
}

/**
 * Get water level name (haut, moyen, bas)
 * 
 * @param integer waterLevelReading
 * @return String
 */
String getWaterLevelName(int waterLevelReading) {
  String waterLevelName;
  
  switch(waterLevelReading) {
    case 3: // Water level high
      changeRgbColor(0, 255, 0);
      waterLevelName = "high";
      break;
    case 2: // Water level middle
      changeRgbColor(255, 255, 0);
      waterLevelName = "middle";
      break;
    case 1: // Water level low
      changeRgbColor(255, 0, 0);
      waterLevelName = "low";
      break;
  }

  return waterLevelName;
}

/**
 * Get ph level by ph sensor value
 * 
 * @param integer phSensorReading
 * @return integer
 */
int getPhLevel(int phSensorReading) {
  float phValue;
  float voltage;
  
  voltage = phSensorReading * 5.0 / 1024;
  phValue = 3.5 * voltage;

  return phValue;
}

/**
 * Change led color by waterlevel
 *   - 3(haut) = green
 *   - 2(milieu) = orange
 *   - 1(bas) = red
 * 
 * @param integer waterLevelReading
 * @return void
 */
void changeLedStatusByWaterLevel(int waterLevelReading) {
  switch(waterLevelReading) {
    case 3: // Water level high
      changeRgbColor(0, 255, 0);
      break;
    case 2: // Water level middle
      changeRgbColor(255, 255, 0);
      break;
    case 1: // Water level low
      changeRgbColor(255, 0, 0);
      break;
  }
}

/**
 * Format datetime to display on screen (dd/mm/YYYY hh:ii)
 * 
 * @param DateTime now
 * @return String
 */
String getDateToDisplay(DateTime now) {
  return String(now.day()) + "/" + String(now.month()) + "/" + String(now.year()) + " " + String(now.hour()) + ":" + String(now.minute());
}

/**
 * Return water sensor value (1=hight, 2=middle, 3=low)
 * 
 * @return integer
 */
int readWaterLevelSensor() {
  digitalWrite(PIN_WATER_LEVEL_LOW, HIGH);
  digitalWrite(PIN_WATER_LEVEL_MIDDLE, HIGH);
  digitalWrite(PIN_WATER_LEVEL_HIGH, HIGH);
  
  delay(100);

  int level = (1-digitalRead(PIN_WATER_LEVEL_LOW)) + (1-digitalRead(PIN_WATER_LEVEL_MIDDLE)) + (1-digitalRead(PIN_WATER_LEVEL_HIGH));

  Serial.print("Water level : ");
  Serial.println(level, DEC);

  digitalWrite(PIN_WATER_LEVEL_LOW, LOW);
  digitalWrite(PIN_WATER_LEVEL_MIDDLE, LOW);
  digitalWrite(PIN_WATER_LEVEL_HIGH, LOW);
  delay(100);

  return level;
}

/**
 * Change rgb led color
 * 
 * @param integer red
 * @param integer green
 * @param integer blue
 * @return void
 */
void changeRgbColor(int red, int green, int blue) {
  Serial.println("RGB(" + String(red) + ", " + String(green) + ", " + String(blue) + ")");
  analogWrite(PIN_LED_R, red);
  analogWrite(PIN_LED_G, green);
  analogWrite(PIN_LED_B, blue);
}
