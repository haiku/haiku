/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "Gravity.h"

#include "ConfigView.h"
#include "GravityView.h"

#include <ScreenSaver.h>
#include <View.h>

#include <stdlib.h>

Gravity::Gravity(BMessage* prefs, image_id imageID)
	:
	BScreenSaver(prefs, imageID)
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
	view->AddChild(new ConfigView(this, view->Bounds()));
}


status_t
Gravity::StartSaver(BView* view, bool preview)
{
	if (preview) {
		fView = NULL;
		return B_ERROR;
	} else {
		SetTickSize((1000 / 20) * 1000);
			// ~20 FPS
		fView = new GravityView(this, view->Bounds());
		view->AddChild(fView);
		return B_OK;
	}
}


void
Gravity::StopSaver()
{
	if (fView != NULL)
		fView->EnableDirectMode(false);
}


void
Gravity::DirectConnected(direct_buffer_info* info)
{
	if (fView != NULL) {
		// TODO: Find out why I had to uncomment this.
		// view->DirectConnected(pdbiInfo);
		// view->EnableDirectMode(true);
	}
}


void
Gravity::DirectDraw(int32 frame)
{
	fView->DirectDraw();
}
