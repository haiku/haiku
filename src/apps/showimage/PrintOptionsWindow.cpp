/*
 * Copyright 2003-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer, laplace@haiku-os.org
 */

#include "PrintOptionsWindow.h"

#include <stdio.h> // for sprintf
#include <stdlib.h> // for atof

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <String.h>

#include "ShowImageConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrintOptionsWindow"


PrintOptions::PrintOptions()
	:
	fOption(kFitToPage),
	fZoomFactor(1.0),
	fDPI(72.0),
	fWidth(1024 / 72.0),
	fHeight(768 / 72.0)
{
}


void
PrintOptions::SetBounds(BRect rect)
{
	fBounds = rect;
}


void
PrintOptions::SetZoomFactor(float f)
{
	fZoomFactor = f;
	fDPI = 72.0 / fZoomFactor;
}


void
PrintOptions::SetDPI(float dpi)
{
	fDPI = dpi;
	fZoomFactor = 72.0 / dpi;
}


void
PrintOptions::SetWidth(float w)
{
	fWidth = w;
	fHeight = (fBounds.Height() + 1) * w / (fBounds.Width() + 1);
}


void
PrintOptions::SetHeight(float h)
{
	fWidth = (fBounds.Width() + 1) * h / (fBounds.Height() + 1);
	fHeight = h;
}


PrintOptionsWindow::PrintOptionsWindow(BPoint at, PrintOptions* options,
	BWindow* listener)
	:
	BWindow(BRect(at.x, at.y, at.x + 300, at.y + 200),
		B_TRANSLATE("Print options"),
		B_TITLED_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fPrintOptions(options),
	fCurrentOptions(*options),
	fListener(listener),
	fStatus(B_ERROR)
{
	AddToSubset(listener);
	Setup();
	Show();
}


PrintOptionsWindow::~PrintOptionsWindow()
{
	BMessage msg(MSG_PRINT);
	msg.AddInt32("status", fStatus);
	fListener.SendMessage(&msg);
}


BRadioButton*
PrintOptionsWindow::AddRadioButton(const char* name,
	const char* label, uint32 what, bool selected)
{
	BRadioButton* button;
	button = new BRadioButton(name, label, new BMessage(what));
	button->SetValue(selected ? B_CONTROL_ON : B_CONTROL_OFF);
	return button;
}


BTextControl*
PrintOptionsWindow::AddTextControl(const char* name,
	const char* label, float value, uint32 what)
{
	BTextControl* text;
	text = new BTextControl(name, label, "", new BMessage(what));
	text->SetModificationMessage(new BMessage(what));
	SetValue(text, value);
	return text;
}


void
PrintOptionsWindow::Setup()
{
	BString value;
	enum PrintOptions::Option op = fCurrentOptions.Option();
	BRadioButton* rbFit;
	BRadioButton* rbZoom;
	BRadioButton* rbDpi;
	BRadioButton* rbResize;
	BBox* line;
	BButton* button;

	rbFit = AddRadioButton("fit_to_page", B_TRANSLATE("Fit image to page"),
		kMsgFitToPageSelected, op == PrintOptions::kFitToPage);

	rbZoom = AddRadioButton("zoom_factor", B_TRANSLATE("Zoom factor in %:"),
		kMsgZoomFactorSelected, op == PrintOptions::kZoomFactor);

	fZoomFactor = AddTextControl("zoom_factor_text", "",
		fCurrentOptions.ZoomFactor() * 100, kMsgZoomFactorChanged);

	rbDpi = AddRadioButton("dpi", B_TRANSLATE("DPI:"), kMsgDPISelected,
		op == PrintOptions::kDPI);

	fDPI = AddTextControl("dpi_text", "", fCurrentOptions.DPI(),
		kMsgDPIChanged);

	rbResize = AddRadioButton("width_and_height",
		B_TRANSLATE("Resize to (in 1/72 inches):"), kMsgWidthAndHeightSelected,
		op == PrintOptions::kWidth || op == PrintOptions::kHeight);

	fWidth = AddTextControl("width", B_TRANSLATE("Width:"),
		fCurrentOptions.Width(), kMsgWidthChanged);

	fHeight = AddTextControl("height", B_TRANSLATE("Height: "),
		fCurrentOptions.Height(), kMsgHeightChanged);

	line = new BBox(B_EMPTY_STRING, B_WILL_DRAW | B_FRAME_EVENTS,
		B_FANCY_BORDER);
	line->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 1));

	button = new BButton("job setup", B_TRANSLATE("Job setup"),
		new BMessage(kMsgJobSetup));
	SetDefaultButton(button);

	const float spacing = be_control_look->DefaultItemSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BGridLayoutBuilder()
				.Add(rbFit, 0, 0)
				.Add(rbZoom, 0, 1)
				.Add(fZoomFactor, 1, 1)
				.Add(rbDpi, 0, 2)
				.Add(fDPI, 1, 2)
				.Add(rbResize, 0, 3)
				)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fWidth)
			.Add(fHeight)
			.AddGlue()
			.SetInsets(22, 0, 0, 0)
		.End()
		.Add(line)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(button)
		.End()
		.SetInsets(spacing, spacing, spacing, spacing)
	);
}


enum PrintOptions::Option
PrintOptionsWindow::MsgToOption(uint32 what)
{
	switch (what) {
		case kMsgFitToPageSelected: return PrintOptions::kFitToPage;
		case kMsgZoomFactorSelected: return PrintOptions::kZoomFactor;
		case kMsgDPISelected: return PrintOptions::kDPI;
		case kMsgWidthAndHeightSelected: return PrintOptions::kWidth;
	}
	return PrintOptions::kFitToPage;
}


bool
PrintOptionsWindow::GetValue(BTextControl* text, float* value)
{
	*value = atof(text->Text());
	return true;
}


void
PrintOptionsWindow::SetValue(BTextControl* text, float value)
{
	BMessage* msg;
	char s[80];
	snprintf(s, sizeof(s), "%0.0f", value);
	// prevent sending a notification when text is set
	msg = new BMessage(*text->ModificationMessage());
	text->SetModificationMessage(NULL);
	text->SetText(s);
	text->SetModificationMessage(msg);
}


void
PrintOptionsWindow::MessageReceived(BMessage* msg)
{
	float value;
	switch (msg->what) {
		case kMsgFitToPageSelected:
		case kMsgZoomFactorSelected:
		case kMsgDPISelected:
		case kMsgWidthAndHeightSelected:
			fCurrentOptions.SetOption(MsgToOption(msg->what));
			break;

		case kMsgZoomFactorChanged:
			if (GetValue(fZoomFactor, &value)
				&& fCurrentOptions.ZoomFactor() != value) {
				fCurrentOptions.SetZoomFactor(value / 100);
				SetValue(fDPI, fCurrentOptions.DPI());
			}
			break;
		case kMsgDPIChanged:
			if (GetValue(fDPI, &value) && fCurrentOptions.DPI() != value) {
				fCurrentOptions.SetDPI(value);
				SetValue(fZoomFactor, 100 * fCurrentOptions.ZoomFactor());
			}
			break;
		case kMsgWidthChanged:
			if (GetValue(fWidth, &value) && fCurrentOptions.Width() != value) {
				fCurrentOptions.SetWidth(value);
				SetValue(fHeight, fCurrentOptions.Height());
			}
			break;
		case kMsgHeightChanged:
			if (GetValue(fHeight, &value) && fCurrentOptions.Height() != value) {
				fCurrentOptions.SetHeight(value);
				SetValue(fWidth, fCurrentOptions.Width());
			}
			break;

		case kMsgJobSetup:
			*fPrintOptions = fCurrentOptions;
			fStatus = B_OK;
			PostMessage(B_QUIT_REQUESTED);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}

