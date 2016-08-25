/*
  CameraShutter.ino - v1.2.0 - 2016-08-24

  CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) 2016 Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  v1.0.x: Depends on TimerOne.h
  v1.1.x: Depends on TimeAlarms.h (hence TimeLib.h) library for Timer purpose (intead of TimerOne.h lib)
          Depends on LcdProgressBarDouble.h (hence LiquidCrystal.h) library for progress bar display
  v1.2.x: Depends on AnalogMultiButton.h to provide values jump, a single input for 2 buttons and code easier to  (+ v1.1.x deps)
*/

#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal.h>
#include <LcdProgressBarDouble.h>
#include <AnalogMultiButton.h>

/** Alarms  */
#define ALARM_ONCE 0
#define ALARM_INTERVAL 1
AlarmId alarmId[2];

LiquidCrystal lcd(12, 11, 5, 4, 3, 8);
LcdProgressBarDouble lpg(&lcd, 1, 16);


//--- AnalogMultiButton defs
const int ANALOG_MULTI_BUTTONS_PIN = A0;
const int BUTTON_PLUS = 1;
const int BUTTON_MINUS = 0;
const int BUTTONS_TOTAL = 3;
const int BUTTONS_VALUES[BUTTONS_TOTAL] = {0, 300, 400};
AnalogMultiButton buttons(ANALOG_MULTI_BUTTONS_PIN, BUTTONS_TOTAL, BUTTONS_VALUES);
//--- /AnalogMultiButton defs

const int shutterPin = 6;
const int focusPin   = 7;

const int switchMenuPin = 2; // INT0
int switchMenuState = LOW;

const String VERSION = "1.2.0";
int nbPictures = 0;

unsigned long startedMillis = 0;

#define MENU_STATUS     0
#define MENU_DELAY      1
#define MENU_DURATION   2
#define MENU_INTERVAL   3
#define MENU_AUTOFOCUS  4
#define MENU_RESET      5

#define NB_MENUS 6

String menus[NB_MENUS][5] = {
    // label, default, min, max, unit
    {"Status",    "0", "0", "2"},
    {"Delay",    "10", "1", "3600", "s"},
    {"Duration", "10", "1", "600", "min"},
    {"Interval",  "2", "2", "3600", "s"},
    {"Autofocus", "1", "0", "1"},
    {"Reset",     "0", "0", "1"}
};

String statuses[3] = {
     "Stopped",
     "Count down",
     "Shooting"
};

int defaultValues[NB_MENUS] = {
    0, // Status
    2, // Delay
    1, // Duration
    7, // Interval
    0, // Autofocus
    0  // Reset
};
int values[NB_MENUS];

volatile byte menusRequested = 0;
volatile boolean isShooting = false;
byte menusCurrent = 0;

void resetValues()
{
    memcpy(values,  defaultValues, sizeof defaultValues);
}

void welcomeMessage()
{
    lcd.setCursor(0, 0);
    lcd.print("Camera Shutter");

    lcd.setCursor(0, 1);
    lcd.print("Version " + VERSION);

    delay(1500); 
}

void setup()
{
    //Serial.begin(115200);
    resetValues();

    pinMode(shutterPin, OUTPUT);
    pinMode(focusPin,   OUTPUT);

    digitalWrite(shutterPin, LOW);
    digitalWrite(focusPin,   LOW);

    lcd.begin(16, 2);

    welcomeMessage();

    attachInterrupt(digitalPinToInterrupt(switchMenuPin), nextMenu, RISING);
    setMenu(menusCurrent);
}

void menuSetupDisplay(int n)
{
    lcd.setCursor(0, 1);
    switch (menusCurrent) {
        case MENU_STATUS:
            if (1 == values[MENU_STATUS]) {
                lcd.setCursor(0, 0);
            }
            lcd.print(statuses[values[n]]);
            break;
        case MENU_DELAY:
        case MENU_DURATION:
        case MENU_INTERVAL:
            lcd.print(values[n]);
            lcd.print(menus[n][4]);
            break;
        case MENU_AUTOFOCUS:
            lcd.print(values[n] ? "yes (once)" : "no        ");
            break;
        case MENU_RESET:
            lcd.print(values[n] ? "yes       " : "no        ");
            break;
    }
    lcd.print("    ");
}

void statusDisplay(long delayMillis)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(statuses[values[MENU_STATUS]]);
    lcd.setCursor(0, 1);
    lcd.print(delayMillis / 1000);
    Alarm.delay(200);
}

void setMenu(int n)
{
    menusCurrent = n;
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(menus[n][0]);

    menuSetupDisplay(n);
}

/**
 * Incr menusRequested (via H/W interruption)
 */
void nextMenu()
{
    menusRequested = (++menusRequested) % NB_MENUS;
    /*
    Serial.print("nextMenu has been pressed: =");
    Serial.print(menusRequested);
    Serial.print(",  menusCurrent=");
    Serial.print(menusCurrent);
    Serial.print(",  values[MENU_STATUS]=");
    Serial.println(values[MENU_STATUS]);
    */
}


void menuSetup()
{
    if (menusRequested != menusCurrent) {
        //--- Next menu pressed
        setMenu(menusRequested);
    }

    //--- BUTTON_PLUS
    int incr = 1;
    if (buttons.isPressedAfter(BUTTON_PLUS, 2000)) {
        //--- Hyper jump!
        incr = 20;
    } else if (buttons.isPressedAfter(BUTTON_PLUS, 500)) {
        //--- Long jump
        incr = 5;
    }
    if (incr > 1 || buttons.onPress(BUTTON_PLUS)) {
        switch (menusCurrent) {
            case MENU_STATUS:
            case MENU_AUTOFOCUS:
            case MENU_RESET:
                //--- Boolean: no sense to jump over values
                incr = 1;
                /* break intentionally avoided */            
            case MENU_DELAY:
            case MENU_DURATION:
            case MENU_INTERVAL:
                // label, default, min, max, unit
                //values[menusCurrent] = ((values[menusCurrent] + incr) % ((menus[menusCurrent][3]).toInt() + 1));
                values[menusCurrent] = values[menusCurrent] + incr;
                if (values[menusCurrent] > (menus[menusCurrent][3]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][3].toInt());
                }

                menuSetupDisplay(menusCurrent);
                break;
        }
        /*
        Serial.print("BUTTON_PLUS,  menusCurrent=");
        Serial.print(menusCurrent);
        Serial.print(",  values[menusCurrent]=");
        Serial.println(values[menusCurrent]);
        */
    }

    if (incr > 1) {
        //--- Relax after a jump :)
        delay(200);
    }

    //--- BUTTON_MINUS
    incr = 1;
    if (buttons.isPressedAfter(BUTTON_MINUS, 2000)) {
        //--- Hyper jump!
        incr = 20;
    } else if (buttons.isPressedAfter(BUTTON_MINUS, 500)) {
        //--- Long jump
        incr = 5;
    }
    if (incr > 1 || buttons.onPress(BUTTON_MINUS)) {
        switch (menusCurrent) {
            case MENU_STATUS:
            case MENU_AUTOFOCUS:
            case MENU_RESET:
                //--- Boolean: no sense to jump over values
                incr = 1;
                /* break intentionally avoided */     
            case MENU_DELAY:
            case MENU_DURATION:
            case MENU_INTERVAL:
                values[menusCurrent] = values[menusCurrent] - incr; //((values[menusCurrent]+1) % ((menus[menusCurrent][3]).toInt() + 1));
                if (values[menusCurrent] < (menus[menusCurrent][2]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][2].toInt());
                }

                menuSetupDisplay(menusCurrent);
                break;
        }
        /*
        Serial.print("BUTTON_MINUS,  menusCurrent=");
        Serial.print(menusCurrent);
        Serial.print(",  values[menusCurrent]=");
        Serial.println(values[menusCurrent]);
        */
    }

    if (incr > 1) {
        //--- Relax after a jump :)
        delay(200);
    }

    if (values[MENU_RESET] == 1) {
        //--- Reset has been requested
        resetValues();
        values[MENU_RESET] = 0;
    }
}

void focus()
{
    digitalWrite(focusPin, HIGH);
    Alarm.delay(800);
    digitalWrite(focusPin, LOW);
}

void shoot()
{
    if (!isShooting) {
        return;
    }
    
    digitalWrite(shutterPin, HIGH);
    nbPictures++;

    Alarm.delay(600);
    digitalWrite(shutterPin, LOW);

    isShooting = false;
}

void startShooting()
{
    unsigned long currentMillis = millis();
    
    values[MENU_STATUS] = 2;
    startedMillis = startedMillis - currentMillis;
    menuSetupDisplay(MENU_STATUS);

    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Shooting: ");
    lcd.print(values[MENU_DURATION]);
    lcd.print(menus[MENU_DURATION][4]);
    
    lcd.setCursor(0, 1);
    if (1 == values[MENU_AUTOFOCUS]) {
        lcd.print("Focussing");
        focus();
    }
    lcd.setCursor(0, 1);
    lcd.print("Started");
    startedMillis = millis();
    
    lpg.setMinValues(startedMillis);
    lpg.setMaxValue2(startedMillis + (unsigned long) values[MENU_INTERVAL] * (unsigned long) 1000);
    
    //--- Init shooting
    nbPictures = 0;
    unsigned long duration = (unsigned long)  values[MENU_DURATION] * (unsigned long) 60000;
    
    lpg.setMaxValue1(startedMillis + duration);
    lpg.draw(startedMillis);
    
    alarmId[ALARM_INTERVAL] = Alarm.timerRepeat(values[MENU_INTERVAL], shootTrigger);
    alarmId[ALARM_ONCE]     = Alarm.timerOnce(values[MENU_DURATION] * 60, stopShooting);
}


void shootTrigger()
{
    if (isShooting) {
      return;
    }
    isShooting = true;
}

void stopShooting()
{
    Alarm.free(alarmId[ALARM_ONCE]);
    Alarm.free(alarmId[ALARM_INTERVAL]);

    isShooting = false;
    values[MENU_STATUS] = 0;

    lcd.clear();
    menusRequested = MENU_STATUS;
    setMenu(MENU_STATUS);

    delay(50);

    lcd.setCursor(0, 1);
    lcd.print(nbPictures);
    lcd.print(" pictures");
    
    delay(500);
}

void loop() {
    buttons.update();
    if (1 == values[MENU_STATUS]) {
        // Count down !!

        if (menusRequested != menusCurrent) {
            //--- Menu button pressed: abort count down
            stopShooting();
        } else {
            lpg.draw(millis());
        }
    } else if (2 == values[MENU_STATUS]) {
        // Started
        if (menusRequested != menusCurrent) {
            //--- Menu button pressed: abort shooting
            stopShooting();
        } else {
            unsigned long currentMillis = millis();
            if (isShooting) {
                startedMillis = millis();
                shoot();
                lpg.setRangeValue2(startedMillis, startedMillis + (unsigned long) values[MENU_INTERVAL] * (unsigned long) 1000);
            }
            
            lpg.draw(currentMillis);
        }
    } else {
        // Stopped: play with the setup
        menuSetup();
        if (1 == values[MENU_STATUS]) {
            // Count down just started!
            startedMillis = millis();

            lpg.setRangeValue1(startedMillis, (unsigned long) startedMillis + (unsigned long) values[MENU_DELAY] * (unsigned long) 1000);
            lpg.disableBar2(); // Do not display the lower bar
            alarmId[ALARM_ONCE] = Alarm.timerOnce(values[MENU_DELAY], startShooting);

            setMenu(MENU_STATUS);
        }
    }

    Alarm.delay(50);
}