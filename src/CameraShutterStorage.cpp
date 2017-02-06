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

#include <EEPROM.h>
#include "CameraShutterStorage.h"

// <<constructor>>
CameraShutterStorage::CameraShutterStorage() {
}

//http://stackoverflow.com/questions/28778625/whats-the-difference-between-and-in-c
bool CameraShutterStorage::add(CameraShutterSerializable& serializable) {
	if (nbItems >= CAMERASHUTTERSTORAGE_MAX_ITEMS) {
		return false;
	}
	serializables[nbItems++] = &serializable;
	return true;
}

int CameraShutterStorage::getErrCode() {
	return errCode;
}

bool CameraShutterStorage::write() {
	address = 0;

	for (int i = 0; i < nbItems; i++) {
		CameraShutterSerializable* serializable = serializables[i];

		StoredData storedData;
		storedData.setName(serializable->getName());
		storedData.version = serializable->getVersion();
		storedData.setData(serializable->serialize());

		EEPROM.put(address, storedData);
		address += sizeof(storedData);

		#ifdef CAMERASHUTTERSTORAGE_DEBUG
			Serial.print("##CameraShutterStorage::write()## name=");
			Serial.print(serializable->getName());
			Serial.print(", version=");
			Serial.println(serializable->getVersion());
			Serial.println(serializable->serialize());
			Serial.println("##CameraShutterStorage::/write()##");
		#endif
	}
	//eepromFill();
	//return res;

	if (address >= EEPROM.length()) {
		eepromClear();
		errCode = ERROR_MEMORY_OUT_OF_BOUNDS;
		return false;
	}
	return false;
}

// ex: data=00000,00030,00010,00020,00001
bool CameraShutterStorage::read() {
	address = 0;
	bool res = true;

	for (int i = 0; i < nbItems; i++) {
		CameraShutterSerializable* serializable = serializables[i];

		String name = serializable->getName();
		int version = serializable->getVersion();

		StoredData storedData;
		EEPROM.get(address, storedData);
		address += sizeof(storedData);


		#ifdef CAMERASHUTTERSTORAGE_DEBUG
			Serial.println("##CameraShutterStorage::read()##");
			Serial.print("name=");
			Serial.print(name);
			Serial.print("<>");
			Serial.println(storedData.name);
			Serial.print(", version=");
			Serial.print(version);
			Serial.print("<>");
			Serial.println(storedData.version);
			Serial.print(", data=");
			Serial.println(storedData.data);
			Serial.println("##CameraShutterStorage::/read()##");
		#endif

		if (name != storedData.name) {
			#ifdef CAMERASHUTTERSTORAGE_DEBUG
				Serial.println("@@CameraShutterStorage::read()@@ Invalid name");
			#endif
			eepromClear();
			errCode = ERROR_INVALID_NAME;
			return false;
		}

		if (version != storedData.version) {
			#ifdef CAMERASHUTTERSTORAGE_DEBUG
				Serial.println("@@CameraShutterStorage::read()@@ Invalid version");
			#endif
			eepromClear();
			errCode = ERROR_INVALID_VERSION;
			return false;
		}

		res = serializable->unserialize(storedData.data);
		if (version != storedData.version) {
			#ifdef CAMERASHUTTERSTORAGE_DEBUG
				Serial.println("@@CameraShutterStorage::read()@@ Invalid data");
			#endif
			eepromClear();
			errCode = ERROR_INVALID_DATA;
			return false;
		}

//		/ *res = res &&* / res = eepromCheck("name",    name);
//		/ *res = res &&* / res= eepromCheck("version", version);
//		/ *res = res &&* / String data = eepromRead("data");


		/*CameraShutterSerializable::SerializedData data = serializable->serialize();
		EEPROM.put(address, data);
		address += sizeof(data);*/
	}
	return res;
}

//00000,00004,00006,00008,00001
bool CameraShutterStorage::eepromFill() {
  for (; address < EEPROM.length(); address++) {
	EEPROM.write(address, 0);
  }
  return true;
}

bool CameraShutterStorage::eepromClear() {
	address = 0;
	return eepromFill();
}

/*
bool CameraShutterStorage::eepromWrite(String key, String value) {
	String str = key + "=" + value + "|";
	//String str = "#TEST#";

	Serial.println("**CameraShutterStorage::eepromWrite()**");
	//Serial.println(str);
	//Serial.print(", len=");
	//Serial.println(str.length());
	for (unsigned int i = 0; i < str.length(); i++) {
		Serial.print(str[i]);
		EEPROM.write(address, str[i]);
		address++;

		if (address == EEPROM.length()) {
			return false;
		}
	}
	Serial.println("**CameraShutterStorage::/eepromWrite()**");

	return true;
}

bool CameraShutterStorage::eepromWrite(String key, int value) {
	char str[6];
	sprintf(str, "%d,", value);

	eepromWrite( key, String(str));
}
*/

