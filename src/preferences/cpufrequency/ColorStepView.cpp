/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */

#include "ColorStepView.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Color Step View"


const int32 kColorBarHeight = 15;

const uint32 kMSGSliderChanged = '&slc';


ColorStepView::ColorStepView(BRect frame)
	: BControl(frame, "ColorStepView", "", new BMessage(kSteppingChanged),
				B_FOLLOW_ALL, B_WILL_DRAW),
	fOffScreenView(NULL),
	fOffScreenBitmap(NULL)
{
	fPerformanceList = new PerformanceList(20, true);

	fSliderPosition = 0;
	fNSteps = 0;
	fLowFreqColor.red = 0;
	fLowFreqColor.blue = 255;
	fLowFreqColor.green = 0;

	fHighFreqColor.red = 255;
	fHighFreqColor.blue = 0;
	fHighFreqColor.green = 0;

	fMinFrequencyLabel = "? MHz";
	fMaxFrequencyLabel = "? MHz";
	_InitView();
}


ColorStepView::~ColorStepView()
{
	delete fPerformanceList;
}


void
ColorStepView::AttachedToWindow()
{
	fSlider->SetTarget(this);

	if (!fOffScreenView) {
		fOffScreenView = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	}
	if (!fOffScreenBitmap) {
		fOffScreenBitmap = new BBitmap(Bounds(), B_CMAP8, true, false);
		Window()->Lock();
		if (fOffScreenBitmap && fOffScreenView)
			fOffScreenBitmap->AddChild(fOffScreenView);
		Window()->Unlock();
	}
}


void
ColorStepView::DetachedFromWindow()
{
	BView::DetachedFromWindow();

	if (fOffScreenBitmap) {
		delete fOffScreenBitmap;
		fOffScreenBitmap = NULL;
		fOffScreenView = NULL;
	}
}


void
ColorStepView::FrameResized(float w,float h)
{
	BView::FrameResized(w, h);

	BRect bounds(Bounds());

	if (bounds.right <= 0.0f || bounds.bottom <= 0.0f)
		return;

	if (fOffScreenBitmap) {
		fOffScreenBitmap->RemoveChild(fOffScreenView);
		delete fOffScreenBitmap;

		fOffScreenView->ResizeTo(bounds.Width(), bounds.Height());

		fOffScreenBitmap = new BBitmap(Bounds(), B_RGBA32, true, false);
		fOffScreenBitmap->AddChild(fOffScreenView);
	}

	Invalidate();
}


void
ColorStepView::GetPreferredSize(float *width, float *height)
{
	*width = Frame().Width();
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	*height = fSlider->Frame().Height();
	*height += kColorBarHeight;
	*height += fontHeight.descent + fontHeight.ascent + 5;
}


void
ColorStepView::Draw(BRect updateRect)
{
	BView *view = NULL;
	if(fOffScreenView){
		view = fOffScreenView;
	}
	else{
		view = this;
	}

	if (!fOffScreenBitmap || !fOffScreenBitmap->Lock())
		return;
	view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	view->FillRect(updateRect);

	BRect colorBarRect =  fSlider->BarFrame();
	colorBarRect.top = 0;
	colorBarRect.bottom = kColorBarHeight;
	colorBarRect.OffsetTo(colorBarRect.left, fSlider->Frame().bottom);

	float pos = 0.0;
	for (int i = fPerformanceList->CountItems() - 1; i >= 0 ; i--) {
		performance_step* perfState = fPerformanceList->ItemAt(i);

		float nextPos = perfState->cpu_usage;
		float width = colorBarRect.Width();

		BRect subRect(colorBarRect);
		subRect.left += pos * width;
		subRect.right = colorBarRect.left + nextPos * width;

		view->SetHighColor(perfState->color);
		view->FillRect(subRect);

		pos = nextPos;
	}
	// draw label
	if (IsEnabled()) {
		view->SetHighColor(0, 0, 0);
	} else {
		view->SetHighColor(tint_color(LowColor(), B_DISABLED_LABEL_TINT));
	}

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float totalFontHeight = fontHeight.descent + fontHeight.ascent;

	view->DrawString(fMinFrequencyLabel.String(),
						BPoint(0.0,
								colorBarRect.bottom + totalFontHeight + 5));

	view->DrawString(fMaxFrequencyLabel.String(),
						BPoint(Bounds().right
								- StringWidth(fMaxFrequencyLabel.String()),
								colorBarRect.bottom	+ totalFontHeight + 5));

	// blit bitmap
	view->Sync();
	fOffScreenBitmap->Unlock();
	DrawBitmap(fOffScreenBitmap, B_ORIGIN);

	BView::Draw(updateRect);
}


void
ColorStepView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMSGSliderChanged:
			fSliderPosition = fSlider->Position();
			_CalculatePerformanceSteps();
			Invalidate();
			break;
		case kSteppingChanged:
			Invoke();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
ColorStepView::SetEnabled(bool enabled)
{
	fSlider->SetEnabled(enabled);
	BControl::SetEnabled(enabled);
}


void
ColorStepView::SetFrequencys(StateList *list)
{
	fStateList = list;
	fNSteps = fStateList->CountItems();
	if (fNSteps >= 2) {
		float minFreq = fStateList->ItemAt(fNSteps - 1)->frequency;
		float maxFreq = fStateList->ItemAt(0)->frequency;
		fMinFrequencyLabel = CreateFrequencyString(minFreq);
		fMaxFrequencyLabel = CreateFrequencyString(maxFreq);
	}

	// fit size of fPerformanceList
	int32 perfNumber = fPerformanceList->CountItems();
	if (perfNumber < fNSteps) {
		for (int i = 0; i < fNSteps - perfNumber; i++)
			fPerformanceList->AddItem(new performance_step);
	}
	else {
		for (int i = 0; i <  perfNumber - fNSteps; i++)
			fPerformanceList->RemoveItemAt(0);
	}
	// and fill the list
	_CalculatePerformanceSteps();

}


PerformanceList*
ColorStepView::GetPerformanceSteps()
{
	return fPerformanceList;
}


BString
ColorStepView::CreateFrequencyString(uint16 frequency)
{
	BString string = "";
	if (frequency >= 1000) {
		char buffer [10];
		sprintf (buffer, "%.1f", float(frequency) / 1000);
		string << buffer;
		string += " GHz";
	}
	else {
		string << frequency;
		string += " MHz";
	}
	return string;
}



float
ColorStepView::UsageOfStep(int32 step, int32 nSteps, float base)
{
	float singleWidth = (1 - base) / (nSteps - 1);
	return base + singleWidth * step;
}


void
ColorStepView::_InitView()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect sliderFrame(Bounds());

	fSlider = new BSlider(sliderFrame, "StepSlider",
							B_TRANSLATE("Step up by CPU usage"),
							new BMessage(kSteppingChanged), 0, 100);
	fSlider->SetModificationMessage(new BMessage(kMSGSliderChanged));

	fSliderPosition = 0.25 - fNSteps * 0.05;
	fSlider->SetPosition(fSliderPosition);
	fSlider->SetLimitLabels("0%", "100%");
	fSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSlider->SetHashMarkCount(5);
	AddChild(fSlider);
}


void
ColorStepView::_CalculatePerformanceSteps()
{
	for (int i = 0; i < fNSteps; i++) {
		// begin with the lowest frequency
		performance_step* perfState = fPerformanceList->ItemAt(fNSteps -1 - i);
		perfState->cpu_usage = _PositonStep(i);
		_ColorStep(i, perfState->color);
	}
}


float
ColorStepView::_PositonStep(int32 step)
{
	if (step >= fNSteps)
		return 1.0;

	return UsageOfStep(step, fNSteps, fSliderPosition);
}


void
ColorStepView::_ColorStep(int32 step, rgb_color &color)
{
	color.red = fLowFreqColor.red
					+ (fHighFreqColor.red - fLowFreqColor.red)
						/ (fNSteps - 1) * step;
	color.green = fLowFreqColor.green
					+ (fHighFreqColor.green - fLowFreqColor.green)
						/ (fNSteps - 1) * step;
	color.blue = fLowFreqColor.blue
					+ (fHighFreqColor.blue - fLowFreqColor.blue)
						/ (fNSteps - 1) * step;
}

