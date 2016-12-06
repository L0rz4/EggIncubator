/*
 Name:		EggIncubator.ino
 Created:	11/8/2016 1:47:09 PM
 Author:	Matej
*/

#include "dht.h"
#include <EEPROMVar.h>
#include <EEPROMex.h>
#include "MenuItem.h"
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_MCP23017.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <math.h>


Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

#define VIN 3.3 // input voltage mV
#define RESOLUTION 32768 // resolution 2^15 15bit
#define VOLTAGE_PER_STEP 188 //uV per step
#define BALANCE_RESISTOR 10000


#define HEATER_RELAY 8
#define VENTILATOR_RELAY 9
#define MOISTURE_RELAY 10


// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

MenuItem *currentMenu;
bool buttonPressed = false;
bool navigateMenu = true;

int valueToChange = 0;
double valueToChangeIncrement = 0.0;
bool saveNewSettings = false;
bool settingsSaved = false;

double temperatureSet = 37.5;
double humiditySet = 75.0;
double tempHysteresisSet = 0.5;
double humidityHysteresisSet = 2.0;
double tempOffset = 0.0;

bool heater_on = false;

unsigned long resettimer = 0;
struct BackColors{
	int color;
	char *name;
};
uint8_t bgColorIdx;
BackColors bgColors[8] = { { 0x00, "No backlight" },{ 0x01, "Red" },{ 0x02, "Green" }, {0x03, "Yellow"}, 
							{0x04, "Blue"}, {0x05, "Violet"}, {0x06, "Teal"}, {0x07, "White"} };

// DHT11/22 Temperature/Humidity sensor
dht DHT;
#define DHT22_PIN 2
unsigned long timerDHT22read = 0;
bool moisturer_on = false;

void setup() {
	Serial.begin(115200);
	pinMode(HEATER_RELAY, OUTPUT);
	pinMode(MOISTURE_RELAY, OUTPUT);
	pinMode(VENTILATOR_RELAY, OUTPUT);
	
	ads.begin();
	lcd.begin(16, 2);


	MenuItem *root = new MenuItem(1, PrintRootMenu, false);
	MenuItem *settings = new MenuItem(2, PrintSettingsRootMenu, false);
	MenuItem *dataMenu = new MenuItem(3, PrintDataRootMenu, false);
	MenuItem *about = new MenuItem(4, PrintAboutRootMenu, false);

	root->AddNewSibling(settings);
	root->AddNewSibling(dataMenu);
	root->AddNewSibling(about);

	settings->AddChild(new MenuItem(21, PrintSettingsTemperatureMenu, true));
	settings->AddChild(new MenuItem(22, PrintSettingsTempHysteresisMenu, true));
	settings->AddChild(new MenuItem(23, PrintSettingsTempOffsetMenu, true));
	settings->AddChild(new MenuItem(23, PrintSettingsHumidityMenu, true));
	settings->AddChild(new MenuItem(24, PrintSettingsHumidHysteresisMenu, true));
	settings->AddChild(new MenuItem(25, PrintSettingsBackgroundMenu, true));
	settings->AddChild(new MenuItem(26, PrintSettingsSaveMenu, true));

	dataMenu->AddChild(new MenuItem(31, PrintDataTemperature1Menu, false));
	dataMenu->AddChild(new MenuItem(32, PrintDataTemperature1RawMenu, false));
	dataMenu->AddChild(new MenuItem(33, PrintDataTemperature2Menu, false));
	dataMenu->AddChild(new MenuItem(34, PrintDataHumidityMenu, false));

	currentMenu = root;
	currentMenu->RequestRefresh();

	temperatureSet = (double)EEPROM.readInt(128) / 10.0;
	tempHysteresisSet = (double)EEPROM.readInt(130) / 10.0;
	humiditySet = (double)EEPROM.readInt(132) / 10.0;
	humidityHysteresisSet = (double)EEPROM.readInt(134) / 10.0;
	bgColorIdx = EEPROM.read(136);
	tempOffset = (double)EEPROM.readInt(137) / 100.0;

	lcd.setBacklight(bgColorIdx);
}

void loop() {
	if (millis() - timerDHT22read > 5000) {
		timerDHT22read = millis();
		ReadDHT22();
	}

	Heater();
	Moisturer();
	MenusAndButtons();
}


void MenusAndButtons() {
	uint8_t buttons = lcd.readButtons();

	if (buttons) {
		if (buttons & BUTTON_LEFT && buttons & BUTTON_RIGHT) {
			if (resettimer == 0) {
				resettimer = millis();
			}

			if (millis() - resettimer > 10000) {
				temperatureSet = 37.5;
				humiditySet = 75.0;
				tempHysteresisSet = 0.5;
				humidityHysteresisSet = 2.0;
				tempOffset = 0.0;
				resettimer = 0;
			}
		}
		else {
			resettimer = 0;
		}
	}
	else {
		resettimer = 0;
	}

	if (buttons && !buttonPressed) {
		buttonPressed = true;

		if (navigateMenu) {
			if (buttons & BUTTON_UP) {
				if (currentMenu->GetPreviousSibling() != nullptr) {
					currentMenu = currentMenu->GetPreviousSibling();
					currentMenu->RequestRefresh();
				}
			}
			if (buttons & BUTTON_DOWN) {
				if (currentMenu->GetNextSibling() != nullptr) {
					currentMenu = currentMenu->GetNextSibling();
					currentMenu->RequestRefresh();
				}
			}
			if (buttons & BUTTON_LEFT) {
				if (currentMenu->GetParent() != nullptr) {
					currentMenu = currentMenu->GetParent();
					currentMenu->RequestRefresh();
				}
			}
			if (buttons & BUTTON_RIGHT) {
				if (currentMenu->GetChild() != nullptr) {
					currentMenu = currentMenu->GetChild();
					currentMenu->RequestRefresh();
				}
			}
			if (buttons & BUTTON_SELECT) {
				if (currentMenu->IsSelectable()) {
					navigateMenu = false;
					lcd.blink();
				}
			}
		}
		else {
			if (buttons & BUTTON_UP) {
				valueToChange = 1;
				currentMenu->RequestRefresh();
			}
			if (buttons & BUTTON_DOWN) {
				valueToChange = -1;
				currentMenu->RequestRefresh();
			}
			if (buttons & BUTTON_SELECT) {
				navigateMenu = true;
				lcd.noBlink();
				valueToChange = 0;

				if (saveNewSettings) {
					saveNewSettings = false;
					EEPROM.writeInt(128, temperatureSet * 10);
					EEPROM.writeInt(130, tempHysteresisSet * 10);
					EEPROM.writeInt(132, humiditySet * 10);
					EEPROM.writeInt(134, humidityHysteresisSet * 10);
					EEPROM.write(136, bgColorIdx);
					EEPROM.writeInt(137, tempOffset * 100);
					settingsSaved = true;
					currentMenu->RequestRefresh();
				}
			}
		}
	}
	if (!buttons) {
		buttonPressed = false;
	}
	Serial.print("Menu id: ");
	Serial.println(currentMenu->GetId());
	currentMenu->ExecuteCallback();

}

void Heater() {
	double currentTemp = ReadTemperature();
	if (temperatureSet - currentTemp > tempHysteresisSet) {
		digitalWrite(HEATER_RELAY, HIGH);
		heater_on = true;
	}

	if (temperatureSet <= currentTemp) {
		digitalWrite(HEATER_RELAY, LOW);
		heater_on = false;
	}
}

double Moisturer() {
	double currentHumidity = DHT.humidity;

	if (humiditySet - currentHumidity > humidityHysteresisSet) {
		digitalWrite(MOISTURE_RELAY, HIGH);
		moisturer_on = true;
	}

	if (humiditySet <= currentHumidity) {
		digitalWrite(MOISTURE_RELAY, LOW);
		moisturer_on = false;
	}
}

double ReadTemperature() {
	int16_t adc0;
	adc0 = ads.readADC_SingleEnded(0);
	double temperature1 = (Thermistor(adc0));

	return temperature1 + tempOffset;
}

void PrintRootMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Temp1: ");
		lcd.setCursor(0, 1);
		lcd.print("Humid:");
	}
	PrintTemperature();
	PrintHumidity();
}

void PrintSettingsRootMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Settings");
		lcd.setCursor(15, 0);
		lcd.print((char)0x3E);
	}
}

void PrintAboutRootMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Egg Incubator");
		lcd.setCursor(0, 1);
		lcd.print("version 1.0.0");
	}
}

void PrintSettingsTemperatureMenu() {
	if (currentMenu->Refresh()) {
		temperatureSet += valueToChange*0.1;
		valueToChange = 0;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Set temperature");
		lcd.setCursor(0, 1);
		lcd.print(temperatureSet);
		
	}
}
void PrintSettingsTempHysteresisMenu() {
	if (currentMenu->Refresh()) {
		tempHysteresisSet += valueToChange*0.1;
		valueToChange = 0;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Set temp hyster");
		lcd.setCursor(0, 1);
		lcd.print(tempHysteresisSet);
	}
}

void PrintSettingsTempOffsetMenu() {
	if (currentMenu->Refresh()) {
		tempOffset += valueToChange*0.01;
		valueToChange = 0;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Set temp offset");
		lcd.setCursor(0, 1);
		lcd.print(tempOffset);
	}
}

void PrintSettingsHumidityMenu() {
	if (currentMenu->Refresh()) {
		humiditySet += valueToChange*1.0;
		valueToChange = 0;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Set humidity");
		lcd.setCursor(0, 1);
		lcd.print(humiditySet);
		lcd.print("%");
	}
}
void PrintSettingsHumidHysteresisMenu() {
	if (currentMenu->Refresh()) {
		humidityHysteresisSet += valueToChange*0.5;
		valueToChange = 0;
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Set humid hyster");
		lcd.setCursor(0, 1);
		lcd.print(humidityHysteresisSet);
		lcd.print("%");
	}
}

void PrintSettingsBackgroundMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		bgColorIdx += valueToChange;
		if (bgColorIdx > 7) {
			bgColorIdx = 0;
		}
		else if (bgColorIdx < 0) {
			bgColorIdx = 7;
		}
		lcd.setBacklight(bgColorIdx);
		valueToChange = 0;
		lcd.setCursor(0, 0);
		lcd.print("Set background");
		lcd.setCursor(0, 1);
		lcd.print(bgColors[bgColorIdx].name);
	}
}

void PrintSettingsSaveMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Save new settings?");
		lcd.setCursor(0, 1);
		if (settingsSaved) {
			lcd.print("Settings saved!");
			settingsSaved = false;
		}
		else {
			saveNewSettings = valueToChange > 0 ? true : false;

			if (saveNewSettings) {
				lcd.print("Yes");
			}
			else {
				lcd.print("No");
			}
		}
	}
}

void PrintDataRootMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Show raw data");
		lcd.setCursor(15, 0);
		lcd.print((char)0x3e);
	}
}

void PrintDataTemperature1Menu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Temperature 1");
	}
	lcd.setCursor(0, 1);
	lcd.print(ReadTemperature());
	lcd.print("\337C");
}

void PrintDataTemperature1RawMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Temperature1 RAW");
		lcd.setCursor(0, 1);
	}
	lcd.setCursor(0, 1);
	lcd.print(ads.readADC_SingleEnded(0));
}

void PrintDataTemperature2Menu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Temperature 2");
		lcd.setCursor(0, 1);
	}
	lcd.setCursor(0, 1);
	lcd.print(DHT.temperature);
	lcd.print("\337C");
}

void PrintDataHumidityMenu() {
	if (currentMenu->Refresh()) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("Humidity");
		lcd.setCursor(0, 1);
	}
	lcd.setCursor(0, 1);
	lcd.print(DHT.humidity);
	lcd.print("%");
}

double Thermistor(int RawADC) {
	double Temp;

	//Temp = log(10000.0*((1024.0 / RawADC - 1)));
	Temp = log(BALANCE_RESISTOR*(VIN / (RawADC*0.000188) - 1));
	//         =log(10000.0/(1024.0/RawADC-1)) // for pull-up configuration
	Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp))* Temp);
	Temp = Temp - 273.15; // Convert Kelvin to Celcius
						  //Temp = (Temp * 9.0) / 5.0 + 32.0; // Convert Celcius to Fahrenheit
	return Temp;
}

void PrintTemperature() {
	double temperature1 = ReadTemperature();

	//	lcd.clear();
	lcd.setCursor(7, 0);
	lcd.print(temperature1);
	lcd.print("\337C");

	lcd.setCursor(15, 0);
	if (heater_on) {
		lcd.print("H");
	}
	else {
		lcd.print(" ");
	}
}

void PrintHumidity() {
	lcd.setCursor(7, 1);
	lcd.print(DHT.humidity, 1);
	lcd.print("%");

	lcd.setCursor(13, 1);
	if (moisturer_on) {
		lcd.print("ON ");
	}
	else {
		lcd.print("OFF");
	}
}

void ReadDHT22() {
	int chk = DHT.read22(DHT22_PIN);
	switch (chk)
	{
	case DHTLIB_OK:
		Serial.print("OK,\t");
		break;
	case DHTLIB_ERROR_CHECKSUM:
		Serial.print("Checksum error,\t");
		break;
	case DHTLIB_ERROR_TIMEOUT:
		Serial.print("Time out error,\t");
		break;
	default:
		Serial.print("Unknown error,\t");
		break;
	}
}