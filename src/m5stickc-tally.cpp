#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>
#include <M5StickCPlus.h>
#include <WiFi.h>
#include <Preferences.h>
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

#include "config.hpp"
#include "m5stickc-tally.h"
#include "Free_Fonts.h" // Include the header file attached to this sketch

#define LED_PIN 10 // see https://github.com/m5stack/M5StickC#pinmap

Preferences preferences;
ATEMstd AtemSwitcher;

int input = 1;
int label = 1;
int previousInput = input;
int previousLabel = label;

boolean previousInputIsProgram = false;
boolean previousInputIsPreview = false;

int orientation = INITIAL_ORIENTATION;
int previousOrientation = orientation;
unsigned long lastOrientationUpdate = millis();

void setup()
{
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, 0); // LOW = on, HIGH = off

	preferences.begin("tally", false);

	input = preferences.getInt("input", 1);
	label = preferences.getInt("label", 1);

	M5.begin(true, true, true);
	Serial.println("Boot");
	M5.IMU.Init(); //M5.MPU6886.Init();
	M5.Lcd.setRotation(orientation);
	M5.Lcd.setCursor(0,20);
	M5.Lcd.setFreeFont(FSS9);

	M5.Lcd.printf("Connecting to\n%s: ", SSID);

	WiFi.begin(SSID, PASS);
	Serial.print("Connecting to WiFi..");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		delay(500);
	}
	Serial.println("Connected to the WiFi network");
	Serial.print("IP:");
	Serial.println(WiFi.localIP());
	

	M5.Lcd.printf(" done\n");

	M5.Lcd.printf("Connecting to ATEM\n<%d.%d.%d.%d>: ", SWITCHER_IP[0], SWITCHER_IP[1], SWITCHER_IP[2], SWITCHER_IP[3]);

	AtemSwitcher.begin(SWITCHER_IP);
	AtemSwitcher.serialOutput(1);
	//AtemSwitcher.connect();

	M5.Lcd.printf("done\n");

	//render(input, label, BLACK, GRAY, 1);
	//render(input, label, GREEN, BLACK, 0);
	//render(input, label, RED, BLACK, 1);

	delay(2000);
}

void loop()
{
	M5.update();

	if (AUTOUPDATE_ORIENTATION)
	{
		if (lastOrientationUpdate + 500 < millis())
		{
			updateOrientation();
			lastOrientationUpdate = millis();
		}
	}

	if (M5.BtnA.wasPressed())
	{
		input = (input % NUM_INPUTS) + 1;
		preferences.putInt("input", input);

		updateOrientation();
	}
	if (M5.BtnB.wasPressed())
	{
		label = (label % NUM_LABELS) + 1;
		preferences.putInt("label", label);

		updateOrientation();
	}

	AtemSwitcher.runLoop();

	boolean inputIsProgram = AtemSwitcher.getProgramTally(input);
	boolean inputIsPreview = AtemSwitcher.getPreviewTally(input);

	if ((input != previousInput) || (label != previousLabel) || (inputIsProgram != previousInputIsProgram) || (inputIsPreview != previousInputIsPreview) || (orientation != previousOrientation))
	{
		if (inputIsProgram && inputIsPreview)
		{
			render(input, label, RED, GREEN, 0);
		}
		else if (inputIsProgram)
		{
			render(input, label, RED, BLACK, 0);
		}
		else if (inputIsPreview)
		{
			render(input, label, GREEN, BLACK, 1);
		}
		else
		{
			render(input, label, BLACK, GRAY, 1);
		}
	}

	previousInput = input;
	previousLabel = label;
	previousInputIsProgram = inputIsProgram;
	previousInputIsPreview = inputIsPreview;
	previousOrientation = orientation;
}

/**
 * Render the input number small in the top left corner and the label number big in the center.
 */
void render(int input, int label, unsigned long int screenColor, unsigned long int labelColor, bool ledValue)
{
	digitalWrite(LED_PIN, ledValue);

	M5.Lcd.fillScreen(screenColor);
	M5.Lcd.setTextColor(labelColor, screenColor);

	M5.Lcd.setTextDatum(TL_DATUM);
	M5.Lcd.drawString(String(input), 1, 1, 2);

	M5.Lcd.setTextDatum(MC_DATUM);
	M5.Lcd.drawString(String(label), M5.Lcd.width() / 2, M5.Lcd.height() / 2, 8);

	float voltage = M5.Axp.GetBatVoltage();
	float current = M5.Axp.GetBatCurrent();

	char batteryStatus[9];
	char chargingIcon = current == 0 ? ' ' : (current > 0 ? '+' : '-');
	sprintf(batteryStatus, "%c %.2fV", chargingIcon, voltage);

	M5.Lcd.setTextDatum(TR_DATUM);
	M5.Lcd.drawString(batteryStatus, M5.Lcd.width() - 1, 1, 1);
}

void updateOrientation()
{
	float accX = 0, accY = 0, accZ = 0;

	M5.IMU.getAccelData(&accX, &accY, &accZ);

	if (accZ < .9)
	{
		if (accX > .6)
		{
			orientation = 1;
		}
		else if (accX < .4 && accX > -.5)
		{
			if (accY > 0)
			{
				orientation = 0;
			}
			else
			{
				orientation = 2;
			}
		}
		else
		{
			orientation = 3;
		}
	}

	if (orientation != previousOrientation)
	{
		M5.Lcd.setRotation(orientation);
	}
}
