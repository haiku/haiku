/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "FilePermissionsView.h"

#include <stdio.h>
#include <stdlib.h>

#include <Beep.h>
#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePermissionsView"


const uint32 kPermissionsChanged = 'prch';
const uint32 kNewOwnerEntered = 'nwow';
const uint32 kNewGroupEntered = 'nwgr';


class RotatedStringView: public BStringView
{
public:
	RotatedStringView(BRect r, const char* name, const char* label)
		: BStringView(r, name, label)
	{
	}

	void Draw(BRect invalidate)
	{
		RotateBy(-M_PI / 5);
		TranslateBy(0, Bounds().Height() / 1.5);
		BStringView::Draw(invalidate);
	}
};


//	#pragma mark - FilePermissionsView


FilePermissionsView::FilePermissionsView(BRect rect, Model* model)
	:
	BView(rect, "FilePermissionsView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fModel(model)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// Constants for the column labels: "User", "Group" and "Other".
	const float kColumnLabelMiddle = 77, kColumnLabelTop = 0,
		kColumnLabelSpacing = 37, kColumnLabelBottom = 39,
		kColumnLabelWidth = 80, kAttribFontHeight = 10;

	BStringView* strView;

	strView = new RotatedStringView(
		BRect(kColumnLabelMiddle - kColumnLabelWidth / 2,
			kColumnLabelTop,
			kColumnLabelMiddle + kColumnLabelWidth / 2,
			kColumnLabelBottom),
		"", B_TRANSLATE("Owner"));
	AddChild(strView);
	strView->SetFontSize(kAttribFontHeight);

	strView = new RotatedStringView(
		BRect(kColumnLabelMiddle - kColumnLabelWidth / 2
				+ kColumnLabelSpacing,
			kColumnLabelTop,
			kColumnLabelMiddle + kColumnLabelWidth / 2 + kColumnLabelSpacing,
			kColumnLabelBottom),
		"", B_TRANSLATE("Group"));
	AddChild(strView);
	strView->SetFontSize(kAttribFontHeight);

	strView = new RotatedStringView(
		BRect(kColumnLabelMiddle - kColumnLabelWidth / 2
				+ 2 * kColumnLabelSpacing,
			kColumnLabelTop,
			kColumnLabelMiddle + kColumnLabelWidth / 2
				+ 2 * kColumnLabelSpacing,
			kColumnLabelBottom),
		"", B_TRANSLATE("Other"));
	AddChild(strView);
	strView->SetFontSize(kAttribFontHeight);

	// Constants for the row labels: "Read", "Write" and "Execute".
	const float kRowLabelLeft = 10, kRowLabelTop = kColumnLabelBottom + 5,
		kRowLabelVerticalSpacing = 18, kRowLabelRight = kColumnLabelMiddle
		- kColumnLabelSpacing / 2 - 5, kRowLabelHeight = 14;

	strView = new BStringView(BRect(kRowLabelLeft, kRowLabelTop,
			kRowLabelRight, kRowLabelTop + kRowLabelHeight),
		"", B_TRANSLATE("Read"));
	AddChild(strView);
	strView->SetAlignment(B_ALIGN_RIGHT);
	strView->SetFontSize(kAttribFontHeight);

	strView = new BStringView(BRect(kRowLabelLeft, kRowLabelTop
			+ kRowLabelVerticalSpacing, kRowLabelRight, kRowLabelTop
			+ kRowLabelVerticalSpacing + kRowLabelHeight),
		"", B_TRANSLATE("Write"));
	AddChild(strView);
	strView->SetAlignment(B_ALIGN_RIGHT);
	strView->SetFontSize(kAttribFontHeight);

	strView = new BStringView(BRect(kRowLabelLeft, kRowLabelTop
			+ 2 * kRowLabelVerticalSpacing, kRowLabelRight, kRowLabelTop
			+ 2 * kRowLabelVerticalSpacing + kRowLabelHeight),
		"", B_TRANSLATE("Execute"));
	AddChild(strView);
	strView->SetAlignment(B_ALIGN_RIGHT);
	strView->SetFontSize(kAttribFontHeight);

	// Constants for the 3x3 check box array.
	const float kLeftMargin = kRowLabelRight + 5,
		kTopMargin = kRowLabelTop - 2,
		kHorizontalSpacing = kColumnLabelSpacing,
		kVerticalSpacing = kRowLabelVerticalSpacing,
		kCheckBoxWidth = 18, kCheckBoxHeight = 18;

	BCheckBox** checkBoxArray[3][3] = {
		{
			&fReadUserCheckBox,
			&fReadGroupCheckBox,
			&fReadOtherCheckBox
		},
		{
			&fWriteUserCheckBox,
			&fWriteGroupCheckBox,
			&fWriteOtherCheckBox
		},
		{
			&fExecuteUserCheckBox,
			&fExecuteGroupCheckBox,
			&fExecuteOtherCheckBox
		}
	};

	for (int32 x = 0; x < 3; x++) {
		for (int32 y = 0; y < 3; y++) {
			*checkBoxArray[y][x] =
				new BCheckBox(BRect(kLeftMargin + kHorizontalSpacing * x,
						kTopMargin + kVerticalSpacing * y,
						kLeftMargin + kHorizontalSpacing * x + kCheckBoxWidth,
						kTopMargin + kVerticalSpacing * y + kCheckBoxHeight),
					"", "", new BMessage(kPermissionsChanged));
			AddChild(*checkBoxArray[y][x]);
		}
	}

	const float kTextControlLeft = 170, kTextControlRight = 270,
		kTextControlTop = kRowLabelTop - 19,
		kTextControlHeight = 14, kTextControlSpacing = 16;

	strView = new BStringView(BRect(kTextControlLeft, kTextControlTop,
		kTextControlRight, kTextControlTop + kTextControlHeight), "",
		B_TRANSLATE("Owner"));
	strView->SetAlignment(B_ALIGN_CENTER);
	strView->SetFontSize(kAttribFontHeight);
	AddChild(strView);

	fOwnerTextControl = new BTextControl(
		BRect(kTextControlLeft,
			kTextControlTop - 2 + kTextControlSpacing,
			kTextControlRight,
			kTextControlTop + kTextControlHeight - 2 + kTextControlSpacing),
		"", "", "", new BMessage(kNewOwnerEntered));
	fOwnerTextControl->SetDivider(0);
	AddChild(fOwnerTextControl);

	strView = new BStringView(BRect(kTextControlLeft,
			kTextControlTop + 5 + 2 * kTextControlSpacing,
			kTextControlRight,
			kTextControlTop + 2 + 2 * kTextControlSpacing
				+ kTextControlHeight),
		"", B_TRANSLATE("Group"));
	strView->SetAlignment(B_ALIGN_CENTER);
	strView->SetFontSize(kAttribFontHeight);
	AddChild(strView);

	fGroupTextControl = new BTextControl(BRect(kTextControlLeft,
			kTextControlTop + 3 * kTextControlSpacing,
			kTextControlRight,
			kTextControlTop + 3 * kTextControlSpacing + kTextControlHeight),
		"", "", "", new BMessage(kNewGroupEntered));
	fGroupTextControl->SetDivider(0);
	AddChild(fGroupTextControl);

	ModelChanged(model);
}


void
FilePermissionsView::ModelChanged(Model* model)
{
	fModel = model;

	bool hideCheckBoxes = false;
	uid_t nodeOwner = 0;
	gid_t nodeGroup = 0;
	mode_t perms = 0;

	if (fModel != NULL) {
		BNode node(fModel->EntryRef());

		if (node.InitCheck() == B_OK) {
			if (fReadUserCheckBox->IsHidden()) {
				fReadUserCheckBox->Show();
				fReadGroupCheckBox->Show();
				fReadOtherCheckBox->Show();
				fWriteUserCheckBox->Show();
				fWriteGroupCheckBox->Show();
				fWriteOtherCheckBox->Show();
				fExecuteUserCheckBox->Show();
				fExecuteGroupCheckBox->Show();
				fExecuteOtherCheckBox->Show();
			}

			if (node.GetPermissions(&perms) == B_OK) {
				fReadUserCheckBox->SetValue((int32)(perms & S_IRUSR));
				fReadGroupCheckBox->SetValue((int32)(perms & S_IRGRP));
				fReadOtherCheckBox->SetValue((int32)(perms & S_IROTH));
				fWriteUserCheckBox->SetValue((int32)(perms & S_IWUSR));
				fWriteGroupCheckBox->SetValue((int32)(perms & S_IWGRP));
				fWriteOtherCheckBox->SetValue((int32)(perms & S_IWOTH));
				fExecuteUserCheckBox->SetValue((int32)(perms & S_IXUSR));
				fExecuteGroupCheckBox->SetValue((int32)(perms & S_IXGRP));
				fExecuteOtherCheckBox->SetValue((int32)(perms & S_IXOTH));
			} else
				hideCheckBoxes = true;

			if (node.GetOwner(&nodeOwner) == B_OK) {
				BString user;
				if (nodeOwner == 0)
					if (getenv("USER") != NULL)
						user << getenv("USER");
					else
						user << "root";
				else
					user << nodeOwner;
				fOwnerTextControl->SetText(user.String());
			} else
				fOwnerTextControl->SetText(B_TRANSLATE("Unknown"));

			if (node.GetGroup(&nodeGroup) == B_OK) {
				BString group;
				if (nodeGroup == 0)
					if (getenv("GROUP") != NULL)
						group << getenv("GROUP");
					else
						group << "0";
				else
					group << nodeGroup;
				fGroupTextControl->SetText(group.String());
			} else
				fGroupTextControl->SetText(B_TRANSLATE("Unknown"));

			// Unless we're root, only allow the owner to transfer the
			// ownership, i.e. disable text controls if uid:s doesn't match:
			thread_id thisThread = find_thread(NULL);
			thread_info threadInfo;
			get_thread_info(thisThread, &threadInfo);
			team_info teamInfo;
			get_team_info(threadInfo.team, &teamInfo);
			if (teamInfo.uid != 0 && nodeOwner != teamInfo.uid) {
				fOwnerTextControl->SetEnabled(false);
				fGroupTextControl->SetEnabled(false);
			} else {
				fOwnerTextControl->SetEnabled(true);
				fGroupTextControl->SetEnabled(true);
			}
		} else
			hideCheckBoxes = true;
	} else
		hideCheckBoxes = true;

	if (hideCheckBoxes) {
		fReadUserCheckBox->Hide();
		fReadGroupCheckBox->Hide();
		fReadOtherCheckBox->Hide();
		fWriteUserCheckBox->Hide();
		fWriteGroupCheckBox->Hide();
		fWriteOtherCheckBox->Hide();
		fExecuteUserCheckBox->Hide();
		fExecuteGroupCheckBox->Hide();
		fExecuteOtherCheckBox->Hide();
	}
}


void
FilePermissionsView::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case kPermissionsChanged:
			if (fModel != NULL) {
				mode_t newPermissions = 0;
				newPermissions
					= (mode_t)((fReadUserCheckBox->Value() ? S_IRUSR : 0)
					| (fReadGroupCheckBox->Value() ? S_IRGRP : 0)
					| (fReadOtherCheckBox->Value() ? S_IROTH : 0)

					| (fWriteUserCheckBox->Value() ? S_IWUSR : 0)
					| (fWriteGroupCheckBox->Value() ? S_IWGRP : 0)
					| (fWriteOtherCheckBox->Value() ? S_IWOTH : 0)

					| (fExecuteUserCheckBox->Value() ? S_IXUSR : 0)
					| (fExecuteGroupCheckBox->Value() ? S_IXGRP :0)
					| (fExecuteOtherCheckBox->Value() ? S_IXOTH : 0));

				BNode node(fModel->EntryRef());

				if (node.InitCheck() == B_OK)
					node.SetPermissions(newPermissions);
				else {
					ModelChanged(fModel);
					beep();
				}
			}
			break;

		case kNewOwnerEntered:
			if (fModel != NULL) {
				uid_t owner;
				if (sscanf(fOwnerTextControl->Text(), "%d", &owner) == 1) {
					BNode node(fModel->EntryRef());
					if (node.InitCheck() == B_OK)
						node.SetOwner(owner);
					else {
						ModelChanged(fModel);
						beep();
					}
				} else {
					ModelChanged(fModel);
					beep();
				}
			}
			break;

		case kNewGroupEntered:
			if (fModel != NULL) {
				gid_t group;
				if (sscanf(fGroupTextControl->Text(), "%d", &group) == 1) {
					BNode node(fModel->EntryRef());
					if (node.InitCheck() == B_OK)
						node.SetGroup(group);
					else {
						ModelChanged(fModel);
						beep();
					}
				} else {
					ModelChanged(fModel);
					beep();
				}
			}
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
FilePermissionsView::AttachedToWindow()
{
	fReadUserCheckBox->SetTarget(this);
	fReadGroupCheckBox->SetTarget(this);
	fReadOtherCheckBox->SetTarget(this);
	fWriteUserCheckBox->SetTarget(this);
	fWriteGroupCheckBox->SetTarget(this);
	fWriteOtherCheckBox->SetTarget(this);
	fExecuteUserCheckBox->SetTarget(this);
	fExecuteGroupCheckBox->SetTarget(this);
	fExecuteOtherCheckBox->SetTarget(this);

	fOwnerTextControl->SetTarget(this);
	fGroupTextControl->SetTarget(this);
}
