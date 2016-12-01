/*
 * Copyright 2014-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "BreakpointEditWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "Team.h"
#include "UserBreakpoint.h"
#include "UserInterface.h"


enum {
	MSG_SET_BREAK_ALWAYS 			= 'sbal',
	MSG_SET_BREAK_ON_CONDITION		= 'sboc',
	MSG_SAVE_BREAKPOINT_SETTINGS 	= 'sbps'
};


BreakpointEditWindow::BreakpointEditWindow(::Team* team,
	UserBreakpoint* breakpoint, UserInterfaceListener* listener,
	BHandler* target)
	:
	BWindow(BRect(), "Edit breakpoint", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fTargetBreakpoint(breakpoint),
	fSaveButton(NULL),
	fCancelButton(NULL),
	fTarget(target)
{
	fTargetBreakpoint->AcquireReference();
}


BreakpointEditWindow::~BreakpointEditWindow()
{
	fTargetBreakpoint->ReleaseReference();
	BMessenger(fTarget).SendMessage(MSG_BREAKPOINT_EDIT_WINDOW_CLOSED);
}


BreakpointEditWindow*
BreakpointEditWindow::Create(::Team* team, UserBreakpoint* breakpoint,
	UserInterfaceListener* listener, BHandler* target)
{
	BreakpointEditWindow* self = new BreakpointEditWindow(
		team, breakpoint, listener, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}

void
BreakpointEditWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_BREAK_ALWAYS:
			fConditionInput->SetEnabled(false);
			break;
		case MSG_SET_BREAK_ON_CONDITION:
			fConditionInput->SetEnabled(true);
			break;
		case MSG_SAVE_BREAKPOINT_SETTINGS:
		{
			if (fConditionRadio->Value() == B_CONTROL_ON) {
				fListener->SetBreakpointConditionRequested(
					fTargetBreakpoint, fConditionInput->Text());
			} else {
				fListener->ClearBreakpointConditionRequested(
					fTargetBreakpoint);
			}
			// fall through
		}
		case B_CANCEL:
			Quit();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}

}


void
BreakpointEditWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
BreakpointEditWindow::_Init()
{
	fConditionInput = new BTextControl(NULL, NULL, NULL);
	BLayoutItem* textLayoutItem = fConditionInput->CreateTextViewLayoutItem();
	textLayoutItem->SetExplicitMinSize(BSize(200.0, B_SIZE_UNSET));
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add((fAlwaysRadio = new BRadioButton("Break always",
				new BMessage(MSG_SET_BREAK_ALWAYS))))
		.AddGroup(B_HORIZONTAL)
			.Add((fConditionRadio = new BRadioButton("Break on condition: ",
				new BMessage(MSG_SET_BREAK_ON_CONDITION))))
			.Add(textLayoutItem)
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add((fSaveButton = new BButton("Save",
				new BMessage(MSG_SAVE_BREAKPOINT_SETTINGS))))
			.Add((fCancelButton = new BButton("Cancel",
				new BMessage(B_CANCEL))))
		.End()
	.End();

	AutoLocker< ::Team> teamLocker(fTeam);
	if (fTargetBreakpoint->HasCondition()) {
		fConditionRadio->SetValue(B_CONTROL_ON);
		fConditionInput->SetText(fTargetBreakpoint->Condition());
	} else {
		fAlwaysRadio->SetValue(B_CONTROL_ON);
		fConditionInput->SetEnabled(false);
	}
}
