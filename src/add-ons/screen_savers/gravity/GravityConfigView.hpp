/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 
#ifndef _GRAVITY_CONFIG_VIEW_HPP_
#define _GRAVITY_CONFIG_VIEW_HPP_


#include "GravityScreenSaver.hpp"

#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Slider.h>
#include <StringItem.h>
#include <StringView.h>
#include <View.h>


class GravityScreenSaver;

class GravityConfigView : public BView
{
public:
	GravityConfigView(GravityScreenSaver* parent, BRect frame);
	
	void AttachedToWindow();
	void MessageReceived(BMessage* pbmMessage);
	
private:	
	GravityScreenSaver* parent;
	BListView* pblvShade;
	BStringView* pbsvShadeText;
	BSlider* pbsParticleCount;
};


#endif /* _GRAVITY_CONFIG_VIEW_HPP_ */
