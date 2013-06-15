/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExceptionConfigWindow.h"

#include <Button.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>

#include <AutoLocker.h>

#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "MessageCodes.h"
#include "UserInterface.h"
#include "Team.h"


enum {
	MSG_STOP_ON_THROWN_EXCEPTION_CHANGED	= 'stec',
	MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED	= 'scec'
};


ExceptionConfigWindow::ExceptionConfigWindow(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Configure Exceptions", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fExceptionThrown(NULL),
	fExceptionCaught(NULL),
	fCloseButton(NULL),
	fTarget(target)
{
}


ExceptionConfigWindow::~ExceptionConfigWindow()
{
	BMessenger(fTarget).SendMessage(MSG_EXCEPTION_CONFIG_WINDOW_CLOSED);
}


ExceptionConfigWindow*
ExceptionConfigWindow::Create(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
{
	ExceptionConfigWindow* self = new ExceptionConfigWindow(team, listener,
		target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}

void
ExceptionConfigWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_STOP_ON_THROWN_EXCEPTION_CHANGED:
		{
			_UpdateThrownBreakpoints(fExceptionThrown->Value()
				== B_CONTROL_ON);
			break;
		}

		case MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED:
		{
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}

}


void
ExceptionConfigWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
ExceptionConfigWindow::_Init()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(fExceptionThrown = new BCheckBox("exceptionThrown",
			"Stop when an exception is thrown", new BMessage(
				MSG_STOP_ON_THROWN_EXCEPTION_CHANGED)))
		.Add(fExceptionCaught = new BCheckBox("exceptionCaught",
			"Stop when an exception is caught", new BMessage(
				MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED)))
		.AddGroup(B_HORIZONTAL, 4.0f)
			.AddGlue()
			.Add(fCloseButton = new BButton("Close", new BMessage(
					B_QUIT_REQUESTED)))
		.End();

	fExceptionThrown->SetTarget(this);
	fExceptionCaught->SetTarget(this);

	// TODO: enable once implemented
	fExceptionCaught->SetEnabled(false);

	fCloseButton->SetTarget(this);


	// check if the exception breakpoints are already installed
	AutoLocker< ::Team> teamLocker(fTeam);
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		it.HasNext();) {
		Image* image = it.Next();

		ImageDebugInfo* info = image->GetImageDebugInfo();
		target_addr_t address;
		if (_FindExceptionFunction(info, address) != B_OK)
			continue;

		if (fTeam->BreakpointAtAddress(address) != NULL) {
			fExceptionThrown->SetValue(B_CONTROL_ON);
			break;
		}
	}
}


void
ExceptionConfigWindow::_UpdateThrownBreakpoints(bool enable)
{
	AutoLocker< ::Team> teamLocker(fTeam);
	for (ImageList::ConstIterator it = fTeam->Images().GetIterator();
		it.HasNext();) {
		Image* image = it.Next();

		ImageDebugInfo* info = image->GetImageDebugInfo();
		target_addr_t address;
		if (_FindExceptionFunction(info, address) != B_OK)
			continue;

		if (enable)
			fListener->SetBreakpointRequested(address, true, true);
		else
			fListener->ClearBreakpointRequested(address);
	}
}


status_t
ExceptionConfigWindow::_FindExceptionFunction(ImageDebugInfo* info,
	target_addr_t& _foundAddress) const
{
	if (info != NULL) {
		FunctionInstance* instance = info->FunctionByName(
			"__cxa_allocate_exception");
		if (instance == NULL)
			instance = info->FunctionByName("__throw(void)");

		if (instance != NULL) {
			_foundAddress = instance->Address();
			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}
