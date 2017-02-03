/**
 CameraShutterSerializable

 CameraShutter.ino is an Arduino program which control a DSLR to take a picture every x seconds for x minutes.

  Copyright (c) Wilfried Loche.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  See file LICENSE.txt for further informations on licensing terms.
 */



#ifndef CAMERASHUTTERSERIALIZABLE_H
#define CAMERASHUTTERSERIALIZABLE_H

#include <stdint.h> // byte def
#include <Arduino.h>

class CameraShutterSerializable
{

public:

	virtual String serialize() = 0;
	virtual bool  unserialize(String data) = 0;

	String getName();
	int getVersion();

protected:
	struct SerializedData {
		int version;
		String name;
	};

	SerializedData serializedData;
};

#endif
