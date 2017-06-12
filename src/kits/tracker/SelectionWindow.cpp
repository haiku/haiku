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


#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuItem.h>
#include <WindowPrivate.h>

#include "AutoLock.h"
#include "ContainerWindow.h"
#include "Commands.h"
#include "Screen.h"
#include "SelectionWindow.h"


const uint32 kSelectButtonPressed = 'sbpr';


//	#pragma mark - SelectionWindow


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SelectionWindow"


SelectionWindow::SelectionWindow(BContainerWindow* window)
	:
	BWindow(BRect(0, 0, 270, 0), B_TRANSLATE("Select"),	B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_V_RESIZABLE
			| B_NO_WORKSPACE_ACTIVATION | B_ASYNCHRONOUS_CONTROLS
			| B_NOT_ANCHORED_ON_ACTIVATE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE),
	fParentWindow(window)
{
	if (window->Feel() & kDesktopWindowFeel) {
		// The window will not show up if we have
		// B_FLOATING_SUBSET_WINDOW_FEEL and use it with the desktop window
		// since it's never in front.
		SetFeel(B_NORMAL_WINDOW_FEEL);
	}

	AddToSubset(fParentWindow);

	BMenu* menu = new BPopUpMenu("popup");
	menu->AddItem(new BMenuItem(B_TRANSLATE("starts with"),	NULL));
	menu->AddItem(new BMenuItem(B_TRANSLATE("ends with"), NULL));
	menu->AddItem(new BMenuItem(B_TRANSLATE("contains"), NULL));
	menu->AddItem(new BMenuItem(B_TRANSLATE("matches wildcard expression"),
		NULL));
	menu->AddItem(new BMenuItem(B_TRANSLATE("matches regular expression"),
		NULL));

	menu->SetLabelFromMarked(true);
	menu->ItemAt(3)->SetMarked(true);
		// Set wildcard matching to default.

	// Set up the menu field
	fMatchingTypeMenuField = new BMenuField("name", B_TRANSLATE("Name"), menu);

	// Set up the expression text control
	fExpressionTextControl = new BTextControl("expression", NULL, "", NULL);

	// Set up the Invert checkbox
	fInverseCheckBox = new BCheckBox("inverse", B_TRANSLATE("Invert"), NULL);
	fInverseCheckBox->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	// Set up the Ignore Case checkbox
	fIgnoreCaseCheckBox = new BCheckBox(
		"ignore", B_TRANSLATE("Ignore case"), NULL);
	fIgnoreCaseCheckBox->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fIgnoreCaseCheckBox->SetValue(1);

	// Set up the Select button
	fSelectButton = new BButton("select", B_TRANSLATE("Select"),
		new BMessage(kSelectButtonPressed));
	fSelectButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fSelectButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fMatchingTypeMenuField)
		.Add(fExpressionTextControl)
		.AddGroup(B_HORIZONTAL)
			.Add(fInverseCheckBox)
			.Add(fIgnoreCaseCheckBox)
			.AddGlue(100.0)
			.Add(fSelectButton)
			.End()
		.End();

	Run();

	Lock();
	MoveCloseToMouse();
	fExpressionTextControl->MakeFocus(true);
	Unlock();
}


void
SelectionWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSelectButtonPressed:
		{
			Hide();
				// Order of posting and hiding important
				// since we want to activate the target
				// window when the message arrives.
				// (Hide is synhcronous, while PostMessage is not.)
				// See PoseView::SelectMatchingEntries().

			BMessage* selectionInfo = new BMessage(kSelectMatchingEntries);
			selectionInfo->AddInt32("ExpressionType", ExpressionType());
			BString expression;
			Expression(expression);
			selectionInfo->AddString("Expression", expression.String());
			selectionInfo->AddBool("InvertSelection", Invert());
			selectionInfo->AddBool("IgnoreCase", IgnoreCase());
			fParentWindow->PostMessage(selectionInfo);
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


bool
SelectionWindow::QuitRequested()
{
	Hide();
	return false;
}


void
SelectionWindow::MoveCloseToMouse()
{
	uint32 buttons;
	BPoint mousePosition;

	ChildAt((int32)0)->GetMouse(&mousePosition, &buttons);
	ConvertToScreen(&mousePosition);

	// Position the window centered around the mouse...
	BPoint windowPosition = BPoint(mousePosition.x - Frame().Width() / 2,
		mousePosition.y	- Frame().Height() / 2);

	// ... unless that's outside of the current screen size:
	BScreen screen;
	windowPosition.x
		= MAX(20, MIN(screen.Frame().right - 20 - Frame().Width(),
		windowPosition.x));
	windowPosition.y = MAX(20,
		MIN(screen.Frame().bottom - 20 - Frame().Height(), windowPosition.y));

	MoveTo(windowPosition);
	SetWorkspaces(1UL << current_workspace());
}


TrackerStringExpressionType
SelectionWindow::ExpressionType() const
{
	if (!fMatchingTypeMenuField->LockLooper())
		return kNone;

	BMenuItem* item = fMatchingTypeMenuField->Menu()->FindMarked();
	if (item == NULL) {
		fMatchingTypeMenuField->UnlockLooper();
		return kNone;
	}

	int32 index = fMatchingTypeMenuField->Menu()->IndexOf(item);

	fMatchingTypeMenuField->UnlockLooper();

	if (index < kStartsWith || index > kRegexpMatch)
		return kNone;

	TrackerStringExpressionType typeArray[] = {	kStartsWith, kEndsWith,
		kContains, kGlobMatch, kRegexpMatch};

	return typeArray[index];
}


void
SelectionWindow::Expression(BString &result) const
{
	if (!fExpressionTextControl->LockLooper())
		return;

	result = fExpressionTextControl->Text();

	fExpressionTextControl->UnlockLooper();
}


bool
SelectionWindow::IgnoreCase() const
{
	if (!fIgnoreCaseCheckBox->LockLooper()) {
		// default action
		return true;
	}

	bool ignore = fIgnoreCaseCheckBox->Value() != 0;

	fIgnoreCaseCheckBox->UnlockLooper();

	return ignore;
}


bool
SelectionWindow::Invert() const
{
	if (!fInverseCheckBox->LockLooper()) {
		// default action
		return false;
	}

	bool inverse = fInverseCheckBox->Value() != 0;

	fInverseCheckBox->UnlockLooper();

	return inverse;
}
