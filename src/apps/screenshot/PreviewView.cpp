/*
 * Copyright 2009, Philippe Saint-Pierre, stpere@gmail.com
 * Distributed under the terms of the MIT License.
 */

#include "PreviewView.h"


#include <ControlLook.h>


PreviewView::PreviewView() 
	: 
	BView("preview", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
}


PreviewView::~PreviewView()
{
}


void
PreviewView::Draw(BRect updateRect)
{
	BRect rect = Frame();
	be_control_look->DrawTextControlBorder(this, rect, rect,
		ui_color(B_PANEL_BACKGROUND_COLOR));
}
