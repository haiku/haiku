/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Copyright 2004-2005 yellowTAB GmbH. All Rights Reserverd.
 * Copyright 2006 Bernd Korz. All Rights Reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		yellowTAB GmbH
 *		Bernd Korz
 *		Michael Pfeiffer
 *		Ryan Leavengood
 */

#include "ResizerWindow.h"

#include <stdlib.h>

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GroupLayoutBuilder.h>
#include <GridLayoutBuilder.h>
#include <Locale.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <Rect.h>
#include <String.h>
#include <TextControl.h>

#include "ShowImageConstants.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ResizerWindow"

ResizerWindow::ResizerWindow(BMessenger target, int32 width, int32 height)
	:
	BWindow(BRect(100, 100, 300, 300), B_TRANSLATE("Resize"),
		B_FLOATING_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fOriginalWidth(width),
	fOriginalHeight(height),
	fTarget(target)
{
	BString widthValue;
	widthValue << width;
	fWidth = new BTextControl("width", B_TRANSLATE("Width:"),
		widthValue.String(), NULL);
	fWidth->SetModificationMessage(new BMessage(kWidthModifiedMsg));

	BString heightValue;
	heightValue << height;
	fHeight = new BTextControl("height",
		B_TRANSLATE("Height:"), heightValue.String(), NULL);
	fHeight->SetModificationMessage(new BMessage(kHeightModifiedMsg));

	fAspectRatio = new BCheckBox("Ratio",
		B_TRANSLATE("Keep original proportions"),
		new BMessage(kWidthModifiedMsg));
	fAspectRatio->SetValue(B_CONTROL_ON);

	fApply = new BButton("apply", B_TRANSLATE("Apply"),
		new BMessage(kApplyMsg));
	fApply->MakeDefault(true);

	const float spacing = be_control_look->DefaultItemSpacing();
	const float labelspacing = be_control_look->DefaultLabelSpacing();

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 0)
		.Add(BGridLayoutBuilder(labelspacing, 0)
				.Add(fWidth->CreateLabelLayoutItem(), 0, 0)
				.Add(fWidth->CreateTextViewLayoutItem(), 1, 0)
				.Add(fHeight->CreateLabelLayoutItem(), 0, 1)
				.Add(fHeight->CreateTextViewLayoutItem(), 1, 1)
				)
		.Add(fAspectRatio)
		.AddGroup(B_HORIZONTAL, 0)
			.AddGlue()
			.Add(fApply)
		.End()
		.SetInsets(spacing, spacing, spacing, spacing)
	);

	fWidth->MakeFocus();

}


ResizerWindow::~ResizerWindow()
{
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
		{
			fAspectRatio->SetValue(B_CONTROL_OFF);
			BString width, height;
			width << message->FindInt32("w");
			height << message->FindInt32("h");
			fWidth->SetText(width.String());
			fHeight->SetText(height.String());
			break;
		}
		case kWidthModifiedMsg:
			if (fAspectRatio->Value() == B_CONTROL_ON) {
				int w = atoi(fWidth->Text());
				int h = (int)((int64)w * (int64) fOriginalHeight
					/ (int64) fOriginalWidth);
				BString height;
				height << h;
				BMessage* msg = new BMessage(*fHeight->ModificationMessage());
				fHeight->SetModificationMessage(NULL);
				fHeight->SetText(height.String());
				fHeight->SetModificationMessage(msg);
			}
			break;
		case kHeightModifiedMsg:
			if (fAspectRatio->Value() == B_CONTROL_ON) {
				int h = atoi(fHeight->Text());
				int w = (int)((int64)h * (int64) fOriginalWidth
					/ (int64) fOriginalHeight);
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


bool
ResizerWindow::QuitRequested()
{
	fTarget.SendMessage(MSG_RESIZER_WINDOW_QUIT);
	return true;
}

