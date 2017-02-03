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

#include "CameraShutterSerializable.h"


String CameraShutterSerializable::getName() {
	return serializedData.name;
}

int CameraShutterSerializable::getVersion() {
	return serializedData.version;
}
