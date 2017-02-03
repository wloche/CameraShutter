/**
 CameraShutterMenu

 CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
 */

#include "CameraShutterMenu.h"

// <<constructor>>
CameraShutterMenu::CameraShutterMenu() {
	reset();
}

void CameraShutterMenu::set(byte item) {
	current = item;
}


byte CameraShutterMenu::get() {
	return current;
}


int CameraShutterMenu::getValue(byte item) {
	/*
	Serial.print("##menuSetup()## getValue, item=");
	Serial.print(item);
	Serial.print(", ter=");
	Serial.println((item == MENU_CURRENT) ? current : item);
	*/

	return values[(item == MENU_CURRENT) ? current : item];
}

int CameraShutterMenu::getStatus() {
	return getValue(MENU_STATUS);
}
String CameraShutterMenu::getStatusLabel() {
	return statuses[values[MENU_STATUS]];
}
void CameraShutterMenu::setStatus(int status) {
	values[MENU_STATUS] = status;
}

String CameraShutterMenu::getLabel() {
	return menus[current][0];
}

String CameraShutterMenu::getUnit(byte item) {
	return menus[(item == MENU_CURRENT) ? current : item][4];
}


bool CameraShutterMenu::isNextRequested() {
	return requested != current;
}

byte CameraShutterMenu::getRequested() {
	return requested;
}


void CameraShutterMenu::reset() {
	memcpy(values, defaultValues, sizeof defaultValues);
#ifdef CAMERASHUTTERMENU_DEBUG
	Serial.println("##CameraShutterMenu::reset()## has been pressed: menusRequested=");
#endif
}

/**
 * Incr menusRequested (via H/W interruption)
 */
void CameraShutterMenu::next() {
	requested = (++requested) % NB_MENUS;
/*
	#ifdef CAMERASHUTTERMENU_DEBUG
		Serial.print("##CameraShutterMenu::next()## has been pressed: requested=");
		Serial.print(requested);
		Serial.print(",  current=");
		Serial.print(current);
		Serial.print(",  values(MENU_STATUS)=");
		Serial.print(values[MENU_STATUS]);
		Serial.println(" ##/CameraShutterMenu::next()##");
	#endif
	*/
}

void CameraShutterMenu::incrValue(int incr) {
	values[current] = values[current] + incr;
	if (values[current] > (menus[current][3]).toInt()) {
		values[current] = (menus[current][3].toInt());
	}
#ifdef CAMERASHUTTERMENU_DEBUG
	Serial.print("##CameraShutterMenu::incrValue()## has been pressed: values(current)=");
	Serial.print(values[current]);
	Serial.println(" ##/CameraShutterMenu::incrValue()##");
#endif
}


void CameraShutterMenu::decrValue(int decr) {
	values[current] = values[current] - decr;
	if (values[current] < (menus[current][2]).toInt()) {
		values[current] = (menus[current][3].toInt());
	}
#ifdef CAMERASHUTTERMENU_DEBUG
	Serial.print("##CameraShutterMenu::decrValue()## has been pressed: values(current)=");
	Serial.print(values[current]);
	Serial.println(" ##/CameraShutterMenu::decrValue()##");
#endif
}
