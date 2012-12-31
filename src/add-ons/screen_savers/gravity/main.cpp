/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Gravity.h"


extern "C" _EXPORT BScreenSaver*
instantiate_screen_saver(BMessage* prefs, image_id imageID)
{
	return new Gravity(prefs, imageID);
}
