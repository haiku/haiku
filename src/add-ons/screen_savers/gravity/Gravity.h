/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Tri-Edge AI
 *		John Scipione, jscipione@gmail.com
 */
#ifndef GRAVITY_SCREEN_SAVER_H
#define GRAVITY_SCREEN_SAVER_H


#include <ScreenSaver.h>


class BMessage;
class BView;

class ConfigView;
class GravityView;


class Gravity : public BScreenSaver {
public:
	struct {
		int32 ShadeID;
		int32 ParticleCount;
	} Config;

							Gravity(BMessage* prefs, image_id imageID);

			status_t		SaveState(BMessage* prefs) const;

			void			StartConfig(BView* view);

			status_t		StartSaver(BView* view, bool preview);
			void			StopSaver();

			void			DirectConnected(direct_buffer_info* info);
			void			Draw(BView*, int32 frame);

private:
			GravityView*	fGravityView;
			ConfigView*		fConfigView;
};


#endif	// GRAVITY_SCREEN_SAVER_H
