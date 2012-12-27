/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */
#ifndef _GRAVITY_SCREEN_SAVER_HPP_
#define _GRAVITY_SCREEN_SAVER_HPP_
 

#include "GravityConfigView.hpp"
#include "GravityView.hpp"
 
#include <OS.h>
#include <ScreenSaver.h>
#include <View.h>
#include <GLView.h>

#include <stdlib.h>
#include <time.h>
#include <math.h>
  

class GravityView;
 
class GravityScreenSaver : public BScreenSaver
{
public:
	struct 
	{
		int32 ShadeID;
		int32 ParticleCount;
	} Config;
	
	GravityScreenSaver(BMessage* pbmPrefs, image_id iidImage);

	status_t SaveState(BMessage* pbmPrefs) const;

	void StartConfig(BView* pbvView);
	
	status_t StartSaver(BView* pbvView, bool bPreview);
	void StopSaver();
		
	void DirectConnected(direct_buffer_info* pdbiInfo);
	void DirectDraw(int32 iFrame);
 	
private:
	GravityView* view;
};


#endif /* _GRAVITY_SCREEN_SAVER_HPP_ */
