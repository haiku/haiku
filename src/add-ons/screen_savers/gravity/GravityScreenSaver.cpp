/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 


#include <View.h>
#include <StringView.h>

#include "GravityScreenSaver.hpp"


GravityScreenSaver::GravityScreenSaver(BMessage* pbmPrefs, image_id iidImage) 
	:
	BScreenSaver(pbmPrefs, iidImage)
{
	srand(time(NULL));
	
	if (pbmPrefs->IsEmpty()) {
		Config.ParticleCount = 1;
		Config.ShadeID = 2;
	} else {
		if (pbmPrefs->FindInt32("ParticleCount", &Config.ParticleCount) != B_OK)
			Config.ParticleCount = 1;	
		
		if (pbmPrefs->FindInt32("ShadeID", &Config.ShadeID) != B_OK)
			Config.ShadeID = 2;	
	}
}


status_t
GravityScreenSaver::SaveState(BMessage* pbmPrefs) const
{
	pbmPrefs->AddInt32("ParticleCount", Config.ParticleCount);
	pbmPrefs->AddInt32("ShadeID", Config.ShadeID);
	return B_OK;	
}


void
GravityScreenSaver::StartConfig(BView* pbvView)
{
	pbvView->AddChild(new GravityConfigView(this, pbvView->Bounds()));
}


status_t
GravityScreenSaver::StartSaver(BView* pbvView, bool bPreview)
{
	if (bPreview) {
		view = NULL;
		return B_ERROR;
	} else {
		SetTickSize((1000 / 30) * 1000); // ~30 FPS
	
		view = new GravityView(this, pbvView->Bounds());
		pbvView->AddChild(view);
	
		return B_OK;
	}
}


void
GravityScreenSaver::StopSaver()
{
	if (view != NULL) {
		view->EnableDirectMode(false);
	}
}


void
GravityScreenSaver::DirectConnected(direct_buffer_info* pdbiInfo)
{
	if (view != NULL) {
		// TODO: Find out why I had to uncomment this.
		// view->DirectConnected(pdbiInfo);
		// view->EnableDirectMode(true);
	}
}


void
GravityScreenSaver::DirectDraw(int32 iFrame)
{
	view->Run();
	// Dummy rect
	BRect rect;
	view->Draw(rect);
}
