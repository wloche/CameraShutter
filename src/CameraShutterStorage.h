/**
 CameraShutterStorage

 CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
 */

#ifndef CAMERASHUTTERSTORAGE_H
#define CAMERASHUTTERSTORAGE_H

//#define CAMERASHUTTERSTORAGE_DEBUG // Uncomment for debug puppose

#include <Arduino.h>
#include "CameraShutterSerializable.h"
#include "CameraShutterMenu.h"

#define CAMERASHUTTERSTORAGE_MAX_ITEMS 2

class CameraShutterStorage
{
public:

	static const int ERROR_INVALID_NAME    = 100;
	static const int ERROR_INVALID_VERSION = 101;
	static const int ERROR_INVALID_DATA    = 102;

	static const int ERROR_MEMORY_OUT_OF_BOUNDS = 200;

	CameraShutterStorage();

	bool add(CameraShutterSerializable& serializable);

	bool write();
	bool read();

	int getErrCode();

private:
	struct StoredData {
		int version;
		/*String*/ char name[10];
		/*String*/ char data[50];

		void setName(String n) {
			char *cstr = new char[n.length() + 1];
			strcpy(name, n.c_str());
			delete [] cstr;

		}
		void setData(String d) {
			char *cstr = new char[d.length() + 1];
			strcpy(data, d.c_str());
			delete [] cstr;
		}
	};

//	CameraShutterMenu /*CameraShutterSerializable*/ serializables[];
	CameraShutterSerializable* serializables[CAMERASHUTTERSTORAGE_MAX_ITEMS];

	int nbItems = 0;
	unsigned int address = 0;

	bool eepromFill();
	bool eepromClear();

	int errCode = 0;

//	bool eepromWrite(String key, String value);
//	bool eepromWrite(String key, int value);

};

#endif
