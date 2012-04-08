/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 *		Alexander von Gluck <kallisti5@unixzen.com>
 */


#include "GLifeConfig.h"

#include <GroupLayoutBuilder.h>
#include <Slider.h>
#include <stdio.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include "GLifeState.h"


// ------------------------------------------------------
//  GLifeConfig Class Constructor Definition
GLifeConfig::GLifeConfig(BRect frame, GLifeState* pglsState)
	:
	BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	m_pglsState(pglsState)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Info text
	BStringView* name = new BStringView(frame, B_EMPTY_STRING,
		"OpenGL \"Game of Life\"", B_FOLLOW_LEFT);
	BStringView* author = new BStringView(frame, B_EMPTY_STRING,
		"by Aaron Hill", B_FOLLOW_LEFT);

	// Sliders
	fGridWidth = new BSlider(frame, "GridWidth",
		"Width of Grid: ",
		new BMessage(e_midGridWidth),
		10, 100, B_BLOCK_THUMB);

	fGridWidth->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridWidth->SetLimitLabels("10", "100");
	fGridWidth->SetValue(pglsState->GridWidth());
	fGridWidth->SetHashMarkCount(10);

	fGridHeight = new BSlider(frame, "GridHeight",
		"Height of Grid: ",
		new BMessage(e_midGridHeight),
		10, 100, B_BLOCK_THUMB);

	fGridHeight->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridHeight->SetLimitLabels("10", "100");
	fGridHeight->SetValue(pglsState->GridHeight());
	fGridHeight->SetHashMarkCount(10);

	fBorder = new BSlider(frame, "Border",
		"Overlap Border: ",
		new BMessage(e_midBorder),
		0, 10, B_BLOCK_THUMB);

	fBorder->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBorder->SetLimitLabels("0", "10");
	fBorder->SetValue(pglsState->Border());
	fBorder->SetHashMarkCount(11);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
			.Add(name)
			.Add(author)
		)
		.AddGlue()
		.Add(fGridWidth)
		.Add(fGridHeight)
		.Add(fBorder)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
	);
}


// ------------------------------------------------------
//  GLifeConfig Class AttachedToWindow Definition
void
GLifeConfig::AttachedToWindow(void)
{
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
