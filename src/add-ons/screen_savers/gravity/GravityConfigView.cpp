/* 
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 * 
 * Authors:
 * 		Tri-Edge AI <triedgeai@gmail.com>
 */ 


#include "GravityConfigView.hpp"


class GravityScreenSaver;

GravityConfigView::GravityConfigView(GravityScreenSaver* parent, BRect frame)
	:
	BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
	this->parent = parent;
	
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BStringView* pbsvTitle = new BStringView(frame, B_EMPTY_STRING, 
		"OpenGL Gravity Effect", B_FOLLOW_LEFT);
														
	BStringView* pbsvAuthor = new BStringView(frame, B_EMPTY_STRING, 
		"by Tri-Edge AI", B_FOLLOW_LEFT);
	
	pbsParticleCount = new BSlider(frame, B_EMPTY_STRING, "Particle Count: ",
		new BMessage('pcnt'), 0, 4, B_BLOCK_THUMB);
	
	pbsvShadeText = new BStringView(frame, B_EMPTY_STRING, "Shade: ", 
		B_FOLLOW_LEFT);
	
	pblvShade = new BListView(frame, B_EMPTY_STRING, B_SINGLE_SELECTION_LIST,
		B_FOLLOW_ALL);
									
	pblvShade->SetSelectionMessage(new BMessage('shds'));
	
	pblvShade->AddItem(new BStringItem("Red"));
	pblvShade->AddItem(new BStringItem("Green"));
	pblvShade->AddItem(new BStringItem("Blue"));
	pblvShade->AddItem(new BStringItem("Orange"));
	pblvShade->AddItem(new BStringItem("Purple"));
	pblvShade->AddItem(new BStringItem("White"));
	pblvShade->AddItem(new BStringItem("Rainbow"));
	
	pblvShade->Select(parent->Config.ShadeID);
	
	BScrollView* scroll = new BScrollView(B_EMPTY_STRING, pblvShade, 
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	
	pbsParticleCount->SetHashMarks(B_HASH_MARKS_BOTTOM);
	pbsParticleCount->SetHashMarkCount(5);
	pbsParticleCount->SetLimitLabels("128", "2048");
	
	pbsParticleCount->SetValue(parent->Config.ParticleCount);
	
	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
				.Add(pbsvTitle)
				.Add(pbsvAuthor)
			)
			.Add(pbsvShadeText)
			.Add(scroll)
			.Add(pbsParticleCount)
			.SetInsets(B_USE_DEFAULT_SPACING, 
				B_USE_DEFAULT_SPACING, 
				B_USE_DEFAULT_SPACING, 
				B_USE_DEFAULT_SPACING)
	);
}


void
GravityConfigView::AttachedToWindow()
{
	pblvShade->SetTarget(this);
	pbsParticleCount->SetTarget(this);	
}


void
GravityConfigView::MessageReceived(BMessage* pbmMessage)
{
	if (pbmMessage->what == 'pcnt') {
		parent->Config.ParticleCount = pbsParticleCount->Value();
	} else if (pbmMessage->what == 'shds') {
		parent->Config.ShadeID = pblvShade->CurrentSelection();	
	} else {
		BView::MessageReceived(pbmMessage);	
	}
}
