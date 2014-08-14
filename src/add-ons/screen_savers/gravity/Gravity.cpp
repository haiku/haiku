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


#include "Gravity.h"

#include "ConfigView.h"
#include "GravityView.h"

#include <ScreenSaver.h>
#include <View.h>

#include <stdlib.h>


Gravity::Gravity(BMessage* prefs, image_id imageID)
	:
	BScreenSaver(prefs, imageID),
	fGravityView(NULL),
	fConfigView(NULL)
{
	srand(time(NULL));

	if (prefs->IsEmpty()) {
		Config.ParticleCount = 1;
		Config.ShadeID = 2;
	} else {
		if (prefs->FindInt32("ParticleCount", &Config.ParticleCount) != B_OK)
			Config.ParticleCount = 1;

		if (prefs->FindInt32("ShadeID", &Config.ShadeID) != B_OK)
			Config.ShadeID = 2;
	}
}


status_t
Gravity::SaveState(BMessage* prefs) const
{
	prefs->AddInt32("ParticleCount", Config.ParticleCount);
	prefs->AddInt32("ShadeID", Config.ShadeID);
	return B_OK;
}


void
Gravity::StartConfig(BView* view)
{
	fConfigView = new ConfigView(view->Bounds(), this);
	view->AddChild(fConfigView);
}


status_t
Gravity::StartSaver(BView* view, bool preview)
{
	SetTickSize((1000 / 20) * 1000);
		// ~20 FPS
	fGravityView = new GravityView(view->Bounds(), this);
	view->AddChild(fGravityView);

	return B_OK;
}


void
Gravity::StopSaver()
{
	if (fGravityView != NULL)
		fGravityView->EnableDirectMode(false);
}


void
Gravity::DirectConnected(direct_buffer_info* info)
{
	if (fGravityView != NULL) {
		fGravityView->DirectConnected(info);
		fGravityView->EnableDirectMode(true);
	}
}


void
Gravity::Draw(BView*, int32 frame)
{
	fGravityView->DirectDraw();
}
