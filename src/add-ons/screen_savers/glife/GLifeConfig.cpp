/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Aaron Hill <serac@hillvisions.com>
 *		Alexander von Gluck <kallisti5@unixzen.com>
 */


#include "GLifeConfig.h"

#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <Slider.h>
#include <stdio.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include "GLifeState.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "GLife ScreenSaver"


// ------------------------------------------------------
//  GLifeConfig Class Constructor Definition
GLifeConfig::GLifeConfig(BRect frame, GLifeState* pglsState)
	:
	BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	m_pglsState(pglsState)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Info text
	BStringView* name = new BStringView(frame, B_EMPTY_STRING,
		B_TRANSLATE("OpenGL \"Game of Life\""), B_FOLLOW_LEFT);
	BStringView* author = new BStringView(frame, B_EMPTY_STRING,
		B_TRANSLATE("by Aaron Hill"), B_FOLLOW_LEFT);

	// Sliders
	fGridDelay = new BSlider(frame, "GridDelay",
		B_TRANSLATE("Grid life delay: "),
		new BMessage(kGridDelay),
		0, 4, B_BLOCK_THUMB);

	fGridDelay->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridDelay->SetLimitLabels(B_TRANSLATE("None"), B_TRANSLATE_COMMENT("4x",
			"This is a factor: the x represents 'times'"));
	fGridDelay->SetValue(pglsState->GridDelay());
	fGridDelay->SetHashMarkCount(5);

	fGridBorder = new BSlider(frame, "GridBorder",
		B_TRANSLATE("Grid border: "),
		new BMessage(kGridBorder),
		0, 10, B_BLOCK_THUMB);

	fGridBorder->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fGridBorder->SetLimitLabels("0", "10");
	fGridBorder->SetValue(pglsState->GridBorder());
	fGridBorder->SetHashMarkCount(11);

	fGridWidth = new BSlider(frame, "GridWidth",
		B_TRANSLATE("Grid width: "),
		new BMessage(kGridWidth),
		10, 100, B_BLOCK_THUMB);

	fGridWidth->SetHashMarks(B_HASH_MARKS_BOTTOM);
	//fGridWidth->SetLimitLabels("10", "100");
	fGridWidth->SetValue(pglsState->GridWidth());
	fGridWidth->SetHashMarkCount(10);

	fGridHeight = new BSlider(frame, "GridHeight",
		B_TRANSLATE("Grid height: "),
		new BMessage(kGridHeight),
		10, 100, B_BLOCK_THUMB);

	fGridHeight->SetHashMarks(B_HASH_MARKS_BOTTOM);
	//fGridHeight->SetLimitLabels("10", "100");
	fGridHeight->SetValue(pglsState->GridHeight());
	fGridHeight->SetHashMarkCount(10);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.Add(BGroupLayoutBuilder(B_VERTICAL, 0)
			.Add(name)
			.Add(author)
		)
		.AddGlue()
		.Add(fGridDelay)
		.Add(fGridBorder)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 0)
			.Add(fGridWidth)
			.Add(fGridHeight)
		)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
	);

	// Do our first label update
	_UpdateLabels();
}


// ------------------------------------------------------
//  GLifeConfig Class AttachedToWindow Definition
void
GLifeConfig::AttachedToWindow(void)
{
	fGridWidth->SetTarget(this);
	fGridHeight->SetTarget(this);
	fGridBorder->SetTarget(this);
	fGridDelay->SetTarget(this);

#ifdef _USE_ASYNCHRONOUS

	m_uiWindowFlags = Window()->Flags();
	Window()->SetFlags(m_uiWindowFlags | B_ASYNCHRONOUS_CONTROLS);

#endif
}


void
GLifeConfig::_UpdateLabels()
{
	BString newLabel;
	newLabel.SetToFormat(B_TRANSLATE("Grid width: %li"), fGridWidth->Value());
	fGridWidth->SetLabel(newLabel);

	newLabel.SetToFormat(B_TRANSLATE("Grid height: %li"), fGridHeight->Value());
	fGridHeight->SetLabel(newLabel);

	newLabel.SetToFormat(B_TRANSLATE("Grid border: %li"), fGridBorder->Value());
	fGridBorder->SetLabel(newLabel);

	int32 delay = fGridDelay->Value();
	if (delay <= 0)
		newLabel = B_TRANSLATE("Grid life delay: none");
	else {
		newLabel.SetToFormat(B_TRANSLATE_CONTEXT("Grid life delay: "
			"%" B_PRId32 "x", "This is a factor: the x represents 'times', "
			"don't translate '%" B_PRId32"'"), delay);
	}
	fGridDelay->SetLabel(newLabel);
}


// ------------------------------------------------------
//  GLifeConfig Class MessageReceived Definition
void
GLifeConfig::MessageReceived(BMessage* pbmMessage)
{
	switch(pbmMessage->what) {
		case kGridWidth:
			m_pglsState->GridWidth() = fGridWidth->Value();
			break;
		case kGridHeight:
			m_pglsState->GridHeight() = fGridHeight->Value();
			break;
		case kGridBorder:
			m_pglsState->GridBorder() = fGridBorder->Value();
			break;
		case kGridDelay:
			m_pglsState->GridDelay() = fGridDelay->Value();
			break;
		default:
			BView::MessageReceived(pbmMessage);
			return;
	}
	_UpdateLabels();
}
