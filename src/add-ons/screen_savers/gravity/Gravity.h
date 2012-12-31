/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GRAVITY_SCREEN_SAVER_H
#define GRAVITY_SCREEN_SAVER_H


#include <ScreenSaver.h>

class GravityView;

class BMessage;
class BView;


class Gravity : public BScreenSaver
{
public:
	struct
	{
		int32 ShadeID;
		int32 ParticleCount;
	} Config;

					Gravity(BMessage* prefs, image_id imageID);

	status_t 		SaveState(BMessage* prefs) const;

	void 			StartConfig(BView* view);

	status_t 		StartSaver(BView* view, bool preview);
	void 			StopSaver();

	void 			DirectConnected(direct_buffer_info* info);
	void 			DirectDraw(int32 frame);

private:
	GravityView* 	fView;
};


#endif
