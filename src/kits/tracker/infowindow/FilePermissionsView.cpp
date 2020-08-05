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

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <Beep.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePermissionsView"


const uint32 kPermissionsChanged = 'prch';
const uint32 kNewOwnerEntered = 'nwow';
const uint32 kNewGroupEntered = 'nwgr';


class RotatedStringView: public BStringView
{
public:
	RotatedStringView(const char* name, const char* label)
		: BStringView(name, label)
	{
		BFont currentFont;
		GetFont(&currentFont);

		currentFont.SetRotation(57);
		SetFont(&currentFont);

		// Get the dimension of the bounding box of the string, taking care
		// of the orientation
		const char* stringArray[1];
		stringArray[0] = label;
		BRect rectArray[1];
		escapement_delta delta = { 0.0, 0.0 };
		currentFont.GetBoundingBoxesForStrings(stringArray, 1, B_SCREEN_METRIC,
			&delta,	rectArray);

		// Adjust the size to avoid partial drawing of first and last chars
		// due to the orientation
		fExplicitSize = BSize(rectArray[0].Width(), rectArray[0].Height()
			+ currentFont.Size() / 2);

		SetExplicitSize(fExplicitSize);
	}

	void Draw(BRect invalidate)
	{
		BFont currentFont;
		GetFont(&currentFont);

		// Small adjustment to draw in the calculated area
		TranslateBy(currentFont.Size() / 1.9 + 1, 0);

		BStringView::Draw(invalidate);
	}

	BSize ExplicitSize()
	{
		return fExplicitSize;
	}

private:
	BSize fExplicitSize;
};


//	#pragma mark - FilePermissionsView


FilePermissionsView::FilePermissionsView(BRect rect, Model* model)
	:
	BView(rect, B_TRANSLATE("Permissions"), B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW),
	fModel(model)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	RotatedStringView* ownerRightLabel = new RotatedStringView("",
		B_TRANSLATE("Owner"));
	RotatedStringView* groupRightLabel = new RotatedStringView("",
		B_TRANSLATE("Group"));
	RotatedStringView* otherRightLabel = new RotatedStringView("",
		B_TRANSLATE("Other"));

	// Get the largest inclined area of the three
	BSize ownerRightLabelSize, groupRightLabelSize, maxSize;

	ownerRightLabelSize = ownerRightLabel->ExplicitSize();
	groupRightLabelSize = groupRightLabel->ExplicitSize();

	maxSize.width = std::max(ownerRightLabelSize.width,
		groupRightLabelSize.width);
	maxSize.width = std::max(maxSize.width,
		otherRightLabel->ExplicitSize().width);

	maxSize.height = std::max(ownerRightLabel->ExplicitSize().height,
		groupRightLabel->ExplicitSize().height);
	maxSize.height = std::max(maxSize.height,
		otherRightLabel->ExplicitSize().height);


	// Set all the component with this size
	ownerRightLabel->SetExplicitSize(maxSize);
	groupRightLabel->SetExplicitSize(maxSize);
	otherRightLabel->SetExplicitSize(maxSize);

	BStringView* readLabel = new BStringView("", B_TRANSLATE("Read"));
	readLabel->SetAlignment(B_ALIGN_RIGHT);

	BStringView* writeLabel = new BStringView("", B_TRANSLATE("Write"));
	writeLabel->SetAlignment(B_ALIGN_RIGHT);

	BStringView* executeLabel = new BStringView("", B_TRANSLATE("Execute"));
	executeLabel->SetAlignment(B_ALIGN_RIGHT);

	// Creating checkbox
	fReadUserCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fReadGroupCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fReadOtherCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));

	fWriteUserCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fWriteGroupCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fWriteOtherCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));

	fExecuteUserCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fExecuteGroupCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));
	fExecuteOtherCheckBox = new BCheckBox("", "",
		new BMessage(kPermissionsChanged));

	fOwnerTextControl = new BTextControl("", B_TRANSLATE("Owner"), "",
		new BMessage(kNewOwnerEntered));
	fGroupTextControl = new BTextControl("", B_TRANSLATE("Group"), "",
		new BMessage(kNewGroupEntered));

	BGroupLayout* groupLayout = new BGroupLayout(B_VERTICAL);

	SetLayout(groupLayout);

	BLayoutBuilder::Group<>(groupLayout)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING, 0.0f)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.Add(ownerRightLabel, 1, 0)
				.Add(groupRightLabel, 2, 0)
				.Add(otherRightLabel, 3, 0)
				.Add(readLabel, 0, 1)
				.Add(writeLabel, 0, 2)
				.Add(executeLabel, 0, 3)
				.Add(fReadUserCheckBox, 1, 1)
				.Add(fReadGroupCheckBox, 2, 1)
				.Add(fReadOtherCheckBox, 3, 1)
				.Add(fWriteUserCheckBox, 1, 2)
				.Add(fWriteGroupCheckBox, 2, 2)
				.Add(fWriteOtherCheckBox, 3, 2)
				.Add(fExecuteUserCheckBox, 1, 3)
				.Add(fExecuteGroupCheckBox, 2, 3)
				.Add(fExecuteOtherCheckBox, 3, 3)
				.AddGlue(0, 4)
			.End()
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.AddGlue(0, 0)
				.AddTextControl(fOwnerTextControl, 0, 1)
				.AddTextControl(fGroupTextControl, 0, 2)
			.End()
			.AddGlue()
		.End()
		.AddGlue();

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
