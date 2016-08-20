/*
  CameraShutter.ino - v1.1.0 - 2016-08-19

  CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) 2016 Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  Depends on TimeAlarms.h (hence TimeLib.h) library for Timer purpose (The 1.0.x previous version was based on TimerOne.h lib)
  Depends on LcdProgressBarDouble.h (hence LiquidCrystal.h) library for progress bar display
*/

#include <TimeLib.h>
#include <TimeAlarms.h>
#include <LiquidCrystal.h>
#include <LcdProgressBarDouble.h>

/** Alarms  */
#define ALARM_ONCE 0
#define ALARM_INTERVAL 1
AlarmId alarmId[2];

LiquidCrystal lcd(12, 11, 5, 4, 3, 8);
LcdProgressBarDouble lpg(&lcd, 1, 16);

const int shutterPin = 6;
const int focusPin   = 7;

const int switchMenuPin = 2; // INT0
int switchMenuState = LOW;

const int switchMinusPin = A1;
int switchMinusState = LOW;

const int switchPlusPin = A0;
int switchPlusState = LOW;

const String VERSION = "1.1.0";
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
    Serial.begin(115200);

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

/*
 * S/W debounce (on plus and minus buttons)
 */
boolean debounce(int buttonPin, boolean last)
{
    boolean current = digitalRead(buttonPin);
    if (current != last) {
        Alarm.delay(5);
        current = digitalRead(buttonPin);
    }
    return current;
}

void menuSetupDisplay(int n)
{
    lcd.setCursor(0, 1);
    switch (menusCurrent) {
        case MENU_STATUS:
            lcd.print(statuses[values[n]]);
            break;
        case MENU_DELAY:
        case MENU_DURATION:
        case MENU_INTERVAL:
            lcd.print(values[n]);
            lcd.print(menus[n][4]);
            break;
        case MENU_AUTOFOCUS:
            lcd.print(values[n] ? "yes (before the series)" : "no");
            break;
        case MENU_RESET:
            lcd.print(values[n] ? "yes" : "no");
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
}


void menuSetup()
{
    if (menusRequested != menusCurrent) {
        //--- Next menu pressed
        setMenu(menusRequested);
    }

    int switchPlusStateCurrent = debounce(switchPlusPin, switchPlusState);
    if (HIGH == switchPlusStateCurrent && LOW == switchPlusState) {
        switch (menusCurrent) {
            case MENU_STATUS:
            case MENU_DELAY:
            case MENU_DURATION:
            case MENU_INTERVAL:
            case MENU_AUTOFOCUS:
            case MENU_RESET:
                // label, default, min, max, unit
                values[menusCurrent] = ((values[menusCurrent]+1) % ((menus[menusCurrent][3]).toInt() + 1));
                if (values[menusCurrent] < (menus[menusCurrent][2]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][2].toInt());
                }

                menuSetupDisplay(menusCurrent);
                break;
        }
    }
    switchPlusState = switchPlusStateCurrent;

    int switchMinusStateCurrent = debounce(switchMinusPin, switchMinusState);
    if (HIGH == switchMinusStateCurrent && LOW == switchMinusState) {
        switch (menusCurrent) {
            case MENU_STATUS:
            case MENU_DELAY:
            case MENU_DURATION:
            case MENU_INTERVAL:
            case MENU_AUTOFOCUS:
            case MENU_RESET:
                // label, default, min, max, unit
                values[menusCurrent] = values[menusCurrent] - 1; //((values[menusCurrent]+1) % ((menus[menusCurrent][3]).toInt() + 1));
                if (values[menusCurrent] < (menus[menusCurrent][2]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][3].toInt());
                }

                menuSetupDisplay(menusCurrent);
                break;
        }
    }
    switchMinusState = switchMinusStateCurrent;

    if (values[MENU_RESET] == 1) {
        //--- Reset has been requested
        resetValues();
        values[MENU_RESET] = 0;
        //Serial.print("Values reseted");
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
    
    lcd.clear();
    Alarm.delay(50);
    values[MENU_STATUS] = 0;
    setMenu(MENU_STATUS);
    menusRequested = menusCurrent;

    lcd.setCursor(0, 1);
    lcd.print(nbPictures);
    lcd.print(" pictures");
    
    Alarm.delay(100);
}

void loop() {
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
            // just started!
            startedMillis = millis();

            lpg.setRangeValue1(startedMillis, (unsigned long) startedMillis + (unsigned long) values[MENU_DELAY] * (unsigned long) 1000);
            lpg.disableBar2(); // Do not display the lower bar
            alarmId[ALARM_ONCE] = Alarm.timerOnce(values[MENU_DELAY], startShooting);

            setMenu(MENU_STATUS);
        }
    }

    Alarm.delay(50);
}