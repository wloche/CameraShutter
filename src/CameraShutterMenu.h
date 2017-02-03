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

#ifndef CAMERASHUTTERMENU_H
#define CAMERASHUTTERMENU_H

#include <stdint.h> // byte def
#include <Arduino.h>

#include "CameraShutterSerializable.h"

//#define CAMERASHUTTERMENU_DEBUG // Uncomment for debug puppose

#define MENU_CURRENT  100
#define MENU_STATUS     0
#define MENU_DELAY      1
#define MENU_DURATION   2
#define MENU_INTERVAL   3
#define MENU_AUTOFOCUS  4
#define MENU_MANUAL     5
#define MENU_RESET      6

#define NB_MENUS 7

class CameraShutterMenu: public CameraShutterSerializable
{


public:
	CameraShutterMenu();

	void set(byte value);
	void reset();

	void setStatus(int status);
	void next();
	bool isNextRequested();


	byte get();
	byte getRequested();
	int  getValue(byte item = MENU_CURRENT);
	int  getStatus();
	String getStatusLabel();
	String getLabel();
	String getUnit(byte item = MENU_CURRENT);


	void incrValue(int incr);
	void decrValue(int decr);

	/** CameraShutterSerializable implementation */
	String serialize();
	bool  unserialize(String data);

protected:
	/** CameraShutterSerializable implementation */
	SerializedData serializedData = { .version = 1, .name = "menu" };

private:
	/** @todo Will: Use a structure... :) */
	String menus[NB_MENUS][5] = {
	    // label, default, min, max, unit
	    {"Status",    "0", "0", "2"},
	    {"Delay",    "10", "1", "3600", "s"},
	    {"Duration", "10", "1", "600", "min"},
	    {"Interval",  "2", "2", "3600", "s"},
	    {"Autofocus", "1", "0", "1"},
	    {"Manual",    "1", "0", "1"},
	    {"Reset",     "0", "0", "1"}
	};

	/** Labels for a given status Id */
	String statuses[3] = {
	     "Stopped",
	     "Count down",
	     "Shooting"
	};

	volatile byte requested = 0;
	int values[NB_MENUS];

	/** @todo Will: Use menus[] 2nd column */
	int defaultValues[NB_MENUS] = {
	    0, // Status
	    2, // Delay
	    1, // Duration
	    7, // Interval
	    0, // Autofocus
	    0, // Manual
	    0  // Reset
	};

	/** Current/active menu */
	byte current = 0;

};

#endif
