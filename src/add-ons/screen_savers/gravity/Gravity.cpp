/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 
 

#include "GravityScreenSaver.hpp"


extern "C" _EXPORT BScreenSaver* 
instantiate_screen_saver(BMessage* pbmPrefs, image_id iidImage)
{
	return new GravityScreenSaver(pbmPrefs, iidImage);
}
