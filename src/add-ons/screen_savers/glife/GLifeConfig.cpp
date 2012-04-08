/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 */


#include "GLifeConfig.h"

#include <stdio.h>
#include <Slider.h>
#include <StringView.h>
#include <View.h>

#include "GLifeState.h"


// ------------------------------------------------------
//  GLifeConfig Class Constructor Definition
GLifeConfig::GLifeConfig(BRect rFrame, GLifeState* pglsState)
	:
	BView(rFrame, "", B_FOLLOW_NONE, B_WILL_DRAW),
	m_pglsState(pglsState)
{
	// Static Labels
	AddChild(new BStringView(BRect(10, 0, 240, 12), B_EMPTY_STRING,
		"OpenGL \"Game of Life\""));
	AddChild(new BStringView(BRect(16, 14, 240, 26), B_EMPTY_STRING,
		"by Aaron Hill"));

	// Sliders
	fGridWidth = new BSlider(BRect(10, 34, 234, 84), "GridWidth",
		"Width of Grid: ",
		new BMessage(e_midGridWidth),
		10, 100, B_BLOCK_THUMB);

	fGridWidth->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridWidth->SetLimitLabels("10", "100");
	fGridWidth->SetValue(pglsState->GridWidth());
	fGridWidth->SetHashMarkCount(10);
	AddChild(fGridWidth);

	fGridHeight = new BSlider(BRect(10, 86, 234, 136), "GridHeight",
		"Height of Grid: ",
		new BMessage(e_midGridHeight),
		10, 100, B_BLOCK_THUMB);

	fGridHeight->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridHeight->SetLimitLabels("10", "100");
	fGridHeight->SetValue(pglsState->GridHeight());
	fGridHeight->SetHashMarkCount(10);
	AddChild(fGridHeight);

	fBorder = new BSlider(BRect(10, 138, 234, 188), "Border",
		"Overlap Border: ",
		new BMessage(e_midBorder),
		0, 10, B_BLOCK_THUMB );

	fBorder->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBorder->SetLimitLabels("0", "10");
	fBorder->SetValue(pglsState->Border());
	fBorder->SetHashMarkCount(11);
	AddChild(fBorder);
}


// ------------------------------------------------------
//  GLifeConfig Class AttachedToWindow Definition
void
GLifeConfig::AttachedToWindow(void)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fGridWidth->SetTarget(this);
	fGridHeight->SetTarget(this);
	fBorder->SetTarget(this);

#ifdef _USE_ASYNCHRONOUS
	
	m_uiWindowFlags = Window()->Flags();
	Window()->SetFlags(m_uiWindowFlags | B_ASYNCHRONOUS_CONTROLS);

#endif
}


// ------------------------------------------------------
//  GLifeConfig Class MessageReceived Definition
void
GLifeConfig::MessageReceived(BMessage* pbmMessage)
{
	char	szNewLabel[64];
	int32	iValue;
	
	switch(pbmMessage->what) {
		case e_midGridWidth:
			m_pglsState->GridWidth() = (iValue = fGridWidth->Value());
			sprintf(szNewLabel, "Width of Grid: %li", iValue);
			fGridWidth->SetLabel(szNewLabel);
			break;
		case e_midGridHeight:
			m_pglsState->GridHeight() = (iValue = fGridHeight->Value());
			sprintf(szNewLabel, "Height of Grid: %li", iValue);
			fGridHeight->SetLabel(szNewLabel);
			break;
		case e_midBorder:
			m_pglsState->Border() = (iValue = fBorder->Value());
			sprintf(szNewLabel, "Overlap Border: %li", iValue);
			fBorder->SetLabel(szNewLabel);
			break;
			
		default:
			BView::MessageReceived(pbmMessage);
			break;
	}
}
