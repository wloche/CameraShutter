/*
  CameraShutter.ino - v1.3.1 - 2017-02-02

  CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.

  v1.0.x: Depends on TimerOne.h
  v1.1.x: Depends on TimeAlarms.h (hence TimeLib.h) library for Timer purpose (intead of TimerOne.h lib)
          Depends on LcdProgressBarDouble.h '1.0.4' (hence LiquidCrystal.h) library for progress bar display
  v1.2.0: Depends on AnalogMultiButton.h to provide values jump, a single input for 2 buttons and code easier to read (+ v1.1.x deps)
  v1.3.0: Class CameraShutterMenu to prepare the EEPROM data writing
  v1.3.1: Implement CameraShutterSerializable (to prepare the EEPROM data writing)
  v1.3.x: EEPROM usage to store your presets
*/

#include <TimeAlarms.h>
#include <LiquidCrystal.h>
#include <LcdProgressBarDouble.h>
#include <AnalogMultiButton.h>

//#define EEPROM_USAGE
#define DEBUG // Comment it out

#include "src/CameraShutterMenu.h"
CameraShutterMenu menu;

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

const String VERSION = "1.3.0";
int nbPictures = 0;

unsigned long startedMillis = 0;

volatile boolean isShooting = false;


// #ifdef EEPROM_USAGE

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
    #ifdef DEBUG
        Serial.begin(115200);
        while (!Serial) {
           ; // wait for serial port to connect. Needed for native USB
        }
    #endif
    
    menu.reset();

    // Tests
    // menu.unserialize("00000,00003,00003,03599,00001,");
    // menu.unserialize("00000,00003,00003,prez");
    // Serial.print(",  serialize=");
    // Serial.println(menu.serialize());
        
    pinMode(shutterPin, OUTPUT);
    pinMode(focusPin,   OUTPUT);

    digitalWrite(shutterPin, LOW);
    digitalWrite(focusPin,   LOW);

    lcd.begin(16, 2);

    welcomeMessage();

    attachInterrupt(digitalPinToInterrupt(switchMenuPin), nextMenu /*menu.next*/, RISING);
    setMenu(menu.get());
}

void menuSetupDisplay(int n)
{
    lcd.setCursor(0, 1);
    switch (menu.get()) {
        case MENU_STATUS:
            if (1 == menu.getValue()) {
                lcd.setCursor(0, 0);
            }
            lcd.print(menu.getStatusLabel());
            break;
        case MENU_DELAY:
        case MENU_DURATION:
        case MENU_INTERVAL:
            lcd.print(menu.getValue());
            lcd.print(menu.getUnit());
            break;
        case MENU_AUTOFOCUS:
            lcd.print(menu.getValue() ? "yes (once)" : "no        ");
            break;
        case MENU_RESET:
            lcd.print(menu.getValue() ? "yes       " : "no        ");
            break;
        case MENU_MANUAL:
            lcd.print("-: Foc, +: Shoot");
            break;
    }
    lcd.print("    ");
}

void statusDisplay(long delayMillis)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(menu.getStatus());
    lcd.setCursor(0, 1);
    lcd.print(delayMillis / 1000);
    Alarm.delay(200);
}

void setMenu(int n)
{
    menu.set(n);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(menu.getLabel());

    menuSetupDisplay(n);
}

/**
 * Incr menusRequested (via H/W interruption)
 * @todo Wil: How to call directly menu.next()?
 */
void nextMenu()
{
    menu.next();
    //menusRequested = (++menusRequested) % NB_MENUS;

    #ifdef DEBUG
        Serial.print("##nextMenu()## has been pressed: menusCurrent=");
        Serial.println(menu.get());
        Serial.print(",  values(current)=");
        Serial.println(menu.getValue());
        Serial.println("##/nextMenu()##");
    #endif
}




void focus()
{
    #ifdef DEBUG
        Serial.println("> Focusing...");
    #endif

    digitalWrite(focusPin, HIGH);
    Alarm.delay(800);
    digitalWrite(focusPin, LOW);
}

void shoot(bool forceShoot = false)
{
    if (!forceShoot && !isShooting) {
        return;
    }

    #ifdef DEBUG
        Serial.println("> Shooting");
    #endif

    digitalWrite(shutterPin, HIGH);
    if (!forceShoot) {
        nbPictures++;
    }

    Alarm.delay(600);
    digitalWrite(shutterPin, LOW);

    isShooting = false;
}

void startShooting()
{
    unsigned long currentMillis = millis();

    #ifdef DEBUG
        Serial.println("!!! Start shooting !!!");
    #endif

    menu.setStatus(2);

    startedMillis = startedMillis - currentMillis;
    menuSetupDisplay(MENU_STATUS);

    lcd.setCursor(0, 0);
    lcd.clear();
    lcd.print("Shooting: ");
    lcd.print(menu.getValue(MENU_DURATION));
    lcd.print(menu.getUnit(MENU_DURATION));

    lcd.setCursor(0, 1);
    if (1 == menu.getValue(MENU_AUTOFOCUS)) {
        lcd.print("Focussing");
        focus();
    }
    lcd.setCursor(0, 1);
    lcd.print("Started");
    startedMillis = millis();

    lpg.setMinValues(startedMillis);
    lpg.setMaxValue2(startedMillis + (unsigned long) menu.getValue(MENU_INTERVAL) * (unsigned long) 1000);

    //--- Init shooting
    nbPictures = 0;
    unsigned long duration = (unsigned long)  menu.getValue(MENU_DURATION) * (unsigned long) 60000;

    lpg.setMaxValue1(startedMillis + duration);
    lpg.draw(startedMillis);

    alarmId[ALARM_INTERVAL] = Alarm.timerRepeat(menu.getValue(MENU_INTERVAL), shootTrigger);
    alarmId[ALARM_ONCE]     = Alarm.timerOnce(menu.getValue(MENU_DURATION) * 60, stopShooting);
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
    menu.setStatus(0);

    lcd.clear();
    //menusRequested = MENU_STATUS;
    menu.set(MENU_STATUS);

    delay(50);

    lcd.setCursor(0, 1);
    lcd.print(nbPictures);
    lcd.print(" pictures");

    delay(500);
}




void menuSetup()
{
    if (menu.isNextRequested() /*menusRequested != menusCurrent*/) {
        //--- Next menu pressed
        setMenu(menu.getRequested());
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
        switch (menu.get()) {
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

                menu.incrValue(incr);
                /*
                values[menusCurrent] = values[menusCurrent] + incr;
                if (values[menusCurrent] > (menus[menusCurrent][3]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][3].toInt());
                }
                */
                /*#ifdef EEPROM_USAGE
                    eepromWriteValues();
                #endif*/

                menuSetupDisplay(menu.get());
                break;
            case MENU_MANUAL:
                shoot(true);
                break;
        }

        #ifdef DEBUG
            Serial.print("##menuSetup()## BUTTON_PLUS,  menusCurrent=");
            Serial.print(menu.get());
            Serial.print(",  values[menusCurrent]=");
            Serial.print(menu.getValue());
            Serial.print(",  serialize=");
            Serial.print(menu.serialize());
            Serial.println("##/menuSetup()##");
        #endif
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
        switch (menu.get()) {
            case MENU_STATUS:
            case MENU_AUTOFOCUS:
            case MENU_RESET:
                //--- Boolean: no sense to jump over values
                incr = 1;
                /* break intentionally avoided */     
            case MENU_DELAY:
            case MENU_DURATION:
            case MENU_INTERVAL:

                menu.decrValue(incr);
                /*
                values[menusCurrent] = values[menusCurrent] - incr; //((values[menusCurrent]+1) % ((menus[menusCurrent][3]).toInt() + 1));
                if (values[menusCurrent] < (menus[menusCurrent][2]).toInt()) {
                    values[menusCurrent] = (menus[menusCurrent][2].toInt());
                }
                */
                /*#ifdef EEPROM_USAGE
                    eepromWriteValues();
                #endif*/
                menuSetupDisplay(menu.get());
                break;
            case MENU_MANUAL:
                focus();
                break;
        }

        #ifdef DEBUG
            Serial.print("##menuSetup()## BUTTON_MINUS,  menusCurrent=");
            Serial.print(menu.get());
            Serial.print(",  values[menusCurrent]=");
            Serial.print(menu.getValue());
            Serial.print(",  serialize=");
            Serial.print(menu.serialize());
            Serial.println("##/menuSetup()##");
        #endif
    }

    if (incr > 1) {
        //--- Relax after a jump :)
        delay(200);
    }

    if (menu.getValue(MENU_RESET) == 1) {
        //--- Reset has been requested
        menu.reset();
    }
}


void loop() {
    buttons.update();
    if (1 == menu.getStatus()) {
        // Count down !!

        if (menu.isNextRequested()) {
            //--- Menu button pressed: abort count down
            stopShooting();
        } else {
            lpg.draw(millis());
        }
    } else if (2 == menu.getStatus()) {
        // Started
        if (menu.isNextRequested()) {
            //--- Menu button pressed: abort shooting
            stopShooting();
        } else {
            unsigned long currentMillis = millis();
            if (isShooting) {
                startedMillis = millis();
                shoot();
                lpg.setRangeValue2(startedMillis, startedMillis + (unsigned long) menu.getValue(MENU_INTERVAL) * (unsigned long) 1000);
            }
            
            lpg.draw(currentMillis);
        }
    } else {
        // Stopped: play with the setup
        menuSetup();
        if (1 == menu.getStatus()) {
            // Count down just started!
            startedMillis = millis();

            lpg.setRangeValue1(startedMillis, (unsigned long) startedMillis + (unsigned long) menu.getValue(MENU_DELAY) * (unsigned long) 1000);
            lpg.disableBar2(); // Do not display the lower bar
            alarmId[ALARM_ONCE] = Alarm.timerOnce(menu.getValue(MENU_DELAY), startShooting);

            setMenu(MENU_STATUS);
        }
    }

    Alarm.delay(50);
}
