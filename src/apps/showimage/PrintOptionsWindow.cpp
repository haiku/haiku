/*****************************************************************************/
// PrintOptionsWindow
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// PrintOptionsWindow.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Box.h>
#include <Button.h>
#include <String.h>
#include <stdio.h> // for sprintf
#include <stdlib.h> // for atof

#include "PrintOptionsWindow.h"
#include "ShowImageConstants.h"

PrintOptions::PrintOptions()
	: fOption(kFitToPage)
	, fZoomFactor(1.0)
	, fDPI(72.0)
	, fWidth(1024 / 72.0)
	, fHeight(768 / 72.0)
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
	fHeight = (fBounds.Height()+1) * w / (fBounds.Width()+1);
}

void
PrintOptions::SetHeight(float h)
{
	fWidth = (fBounds.Width()+1) * h / (fBounds.Height()+1);	
	fHeight = h;
}

PrintOptionsWindow::PrintOptionsWindow(BPoint at, PrintOptions *options, BWindow* listener)
	: BWindow(BRect(at.x, at.y, at.x+300, at.y+200), "Print Options", 
	B_TITLED_WINDOW_LOOK, B_MODAL_SUBSET_WINDOW_FEEL, 
	B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
	, fPrintOptions(options)
	, fCurrentOptions(*options)
	, fListener(listener)
	, fStatus(B_ERROR)
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
PrintOptionsWindow::AddRadioButton(BView* view, BPoint& at, const char* name, const char* label, uint32 what, bool selected)
{
	BRect rect(0, 0, 100, 20);
	BRadioButton* button;
	rect.OffsetBy(at);
	button = new BRadioButton(rect, name, label, new BMessage(what));
	view->AddChild(button);
	button->ResizeToPreferred();
	at.y += button->Bounds().Height() + kLineSkip;
	button->SetValue(selected ? B_CONTROL_ON : B_CONTROL_OFF);
	return button;
}

BTextControl*
PrintOptionsWindow::AddTextControl(BView* view, BPoint& at, const char* name, const char* label, float value, float divider, uint32 what)
{
	BRect rect(0, 0, divider + 45, 20);
	BTextControl* text;
	rect.OffsetBy(at);
	text = new BTextControl(rect, name, label, "", new BMessage(what));
	view->AddChild(text);
	text->SetModificationMessage(new BMessage(what));
	text->SetDivider(divider);
	text->SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
	SetValue(text, value);
	at.y += text->Bounds().Height() + kLineSkip;
	return text;	
}

void
PrintOptionsWindow::Setup()
{
	BRect rect(Bounds());
	BPoint at(kIndent, kIndent), textAt;
	BString value;
	enum PrintOptions::Option op = fCurrentOptions.Option();
	BRadioButton* rb;
	BBox* line;
	BButton* button;
	
	BBox *panel = new BBox(rect, "top_panel", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		B_PLAIN_BORDER);
	AddChild(panel);
	
	AddRadioButton(panel, at, "fit_to_page", "Fit Image to Page", kMsgFitToPageSelected, op == PrintOptions::kFitToPage);
	textAt = at;
	rb = AddRadioButton(panel, at, "zoom_factor", "Zoom Factor in %: ", kMsgZoomFactorSelected, op == PrintOptions::kZoomFactor);
	textAt.x = rb->Bounds().right + 5;
	fZoomFactor = AddTextControl(panel, textAt, "zoom_factor_text", "", fCurrentOptions.ZoomFactor()*100, 0, kMsgZoomFactorChanged);

	textAt = at;
	rb = AddRadioButton(panel, at, "dpi", "DPI: ", kMsgDPISelected, op == PrintOptions::kDPI);
	textAt.x = rb->Bounds().right + 5;
	fDPI = AddTextControl(panel, textAt, "dpi_text", "", fCurrentOptions.DPI(), 0, kMsgDPIChanged);

	rb = AddRadioButton(panel, at, "width_and_height", "Resize To (in 1/72 Inches):", kMsgWidthAndHeightSelected, op == PrintOptions::kWidth || op == PrintOptions::kHeight); 
	at.x += 15;
	textAt = at;	
	fWidth = AddTextControl(panel, textAt, "width", "Width: ", fCurrentOptions.Width(), 40, kMsgWidthChanged);
	textAt = at;
	textAt.x += fWidth->Bounds().Width() + 5;
	fHeight = AddTextControl(panel, textAt, "height", "Height: ", fCurrentOptions.Height(), 40, kMsgHeightChanged);

	at.x = 0;
	at.y = textAt.y;
	line = new BBox(BRect(rect.left+3, at.y, rect.right-3, at.y + 1), NULL,
		B_FOLLOW_LEFT | B_FOLLOW_TOP);
	panel->AddChild(line);
	
	at.y += 10;
	rect.OffsetBy(at);
	button = new BButton(rect, "job setup", "Job Setup", new BMessage(kMsgJobSetup));
	panel->AddChild(button);
	button->ResizeToPreferred();

	SetDefaultButton(button);

	// resize window
	ResizeTo(fHeight->Frame().right + kIndent, button->Frame().bottom + kIndent);

	// center button 
	button->MoveTo((Bounds().Width()-button->Bounds().Width())/2, button->Frame().top);
}

enum PrintOptions::Option
PrintOptionsWindow::MsgToOption(uint32 what) {
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
	sprintf(s, "%0.0f", value);
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
			if (GetValue(fZoomFactor, &value) && fCurrentOptions.ZoomFactor() != value) {
				fCurrentOptions.SetZoomFactor(value/100);
				SetValue(fDPI, fCurrentOptions.DPI());
			}
			break;
		case kMsgDPIChanged:
			if (GetValue(fDPI, &value) && fCurrentOptions.DPI() != value) {
				fCurrentOptions.SetDPI(value);
				SetValue(fZoomFactor, 100*fCurrentOptions.ZoomFactor());
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
