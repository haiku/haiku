/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		yellowTAB GmbH
 *		Bernd Korz
 *      Michael Pfeiffer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <Rect.h>
#include <String.h>
#include <TextControl.h>

#include <stdlib.h>

#include "ResizerWindow.h"
#include "ShowImageConstants.h"

static const char* kWidthLabel = "Width:";
static const char* kHeightLabel = "Height:";
static const char* kKeepAspectRatioLabel = "Keep Original Proportions";
static const char* kApplyLabel = "Apply";

static const float kLineDistance = 5;
static const float kHorizontalIndent = 10;
static const float kVerticalIndent = 10;

ResizerWindow::ResizerWindow(BMessenger target, int32 width, int32 height)
	: BWindow(BRect(100, 100, 300, 300), "Resize", B_FLOATING_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE)
	, fOriginalWidth(width)
	, fOriginalHeight(height)
	, fTarget(target)
{
	BView* back_view = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
	back_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(back_view);
	
	const float widthLabelWidth = back_view->StringWidth(kWidthLabel);
	const float heightLabelWidth = back_view->StringWidth(kHeightLabel);
	const float column2 = max_c(widthLabelWidth, heightLabelWidth);

	const float textControlWidth = column2 + back_view->StringWidth("999999");
	const float keepAspectRatioLabelWidth = 20 + back_view->StringWidth(kKeepAspectRatioLabel);
	const float width2 = 2 * kHorizontalIndent + max_c(textControlWidth, keepAspectRatioLabelWidth);
	
	ResizeTo(width2+1, Bounds().Height()+1);
	
	const float top = kVerticalIndent;
	const float left = kHorizontalIndent;
	BRect rect(left, top, width2 - kHorizontalIndent, top + 10);
	
	BString widthValue;
	widthValue << width;
	fWidth = new BTextControl(rect, "width", kWidthLabel, widthValue.String(), NULL);
	fWidth->SetModificationMessage(new BMessage(kWidthModifiedMsg));
	AddControl(back_view, fWidth, column2, rect);
					
	BString heightValue;
	heightValue << height;	
	fHeight = new BTextControl(rect, "height", kHeightLabel, heightValue.String(), NULL);
	fHeight->SetModificationMessage(new BMessage(kHeightModifiedMsg));
	AddControl(back_view, fHeight, column2, rect);
	
	fAspectRatio = new BCheckBox(rect, "Ratio", kKeepAspectRatioLabel, new BMessage(kWidthModifiedMsg));
	fAspectRatio->SetValue(B_CONTROL_ON);
	AddControl(back_view, fAspectRatio, column2, rect);
	
	fApply = new BButton(rect, "apply", kApplyLabel, new BMessage(kApplyMsg));
	fApply->MakeDefault(true);
	AddControl(back_view, fApply, column2, rect);
	LeftAlign(fApply);
	
	fWidth->MakeFocus();
	
	ResizeTo(width2, rect.top);
}

void
ResizerWindow::AddControl(BView* view, BControl* control, float column2, BRect& rect)
{
	float width, height;
	view->AddChild(control);
	control->GetPreferredSize(&width, &height);
	if (dynamic_cast<BButton*>(control) != NULL) {
		control->ResizeTo(width, height);
	} else {
		control->ResizeTo(control->Bounds().Width(), height);
	}
	float top = control->Frame().bottom + kLineDistance;
	rect.OffsetTo(rect.left, top);

	if (dynamic_cast<BTextControl*>(control) != NULL) {
		((BTextControl*)control)->SetDivider(column2);
	}
}

void
ResizerWindow::AddSeparatorLine(BView* view, BRect& rect)
{
	const float lineWidth = 3;
	BRect line(Bounds());
	line.left += 3;
	line.right -= 3;
	line.top = rect.top;
	line.bottom = line.top + lineWidth - 1;
	BBox* separatorLine = new BBox(line, "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER);
	view->AddChild(separatorLine);
	rect.OffsetBy(0, kLineDistance + lineWidth);
}

void
ResizerWindow::LeftAlign(BControl* control)
{
	BRect frame = control->Frame();
	float left = Bounds().Width() - frame.Width() - kHorizontalIndent;
	control->MoveTo(left, frame.top);
}

void 
ResizerWindow::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		// public actions
		case kActivateMsg:
			Activate();
			break;
			
		case kUpdateMsg:
		{
			// update aspect ratio, width and height
			int32 width, height;
			if (message->FindInt32("width", &width) == B_OK &&
				message->FindInt32("height", &height) == B_OK) {
				
				fOriginalWidth = width;
				fOriginalHeight = height;				

				BString widthText, heightText;
				widthText << width;
				heightText << height;
				// here the statement order is important:
				// in case keep aspect ratio is enabled,
				// the width should determine the height
				fHeight->SetText(heightText.String());				
				fWidth->SetText(widthText.String());
			}
			break;
		}
			
		// private actions
		case kResolutionMsg:
		{	fAspectRatio->SetValue(B_CONTROL_OFF);
			BString width, height;
			width << message->FindInt32("w");
			height << message->FindInt32("h");
			fWidth->SetText(width.String());
			fHeight->SetText(height.String());
			break;
		}
		case kWidthModifiedMsg:
			if (fAspectRatio->Value() == B_CONTROL_ON)
			{
				int w = atoi(fWidth->Text());
				int h = (int)((int64)w * (int64) fOriginalHeight / (int64) fOriginalWidth);
				BString height;
				height << h;
				BMessage* msg = new BMessage(*fHeight->ModificationMessage());
				fHeight->SetModificationMessage(NULL);
				fHeight->SetText(height.String());
				fHeight->SetModificationMessage(msg);
			}
			break;
		case kHeightModifiedMsg:
			if (fAspectRatio->Value() == B_CONTROL_ON)
			{
				int h = atoi(fHeight->Text());
				int w = (int)((int64)h * (int64) fOriginalWidth / (int64) fOriginalHeight);
				BString width;
				width << w;
				BMessage* msg = new BMessage(*fWidth->ModificationMessage());
				fWidth->SetModificationMessage(NULL);
				fWidth->SetText(width.String());
				fWidth->SetModificationMessage(msg);
			}
			break;
		case kApplyMsg:
		{
			BMessage resizeRequest(MSG_RESIZE);
			resizeRequest.AddInt32("h", atoi(fHeight->Text()));
			resizeRequest.AddInt32("w", atoi(fWidth->Text()));
			fTarget.SendMessage(&resizeRequest);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

ResizerWindow::~ResizerWindow()
{
}

bool
ResizerWindow::QuitRequested()
{
	fTarget.SendMessage(MSG_RESIZER_WINDOW_QUITED);
	return true;
}
