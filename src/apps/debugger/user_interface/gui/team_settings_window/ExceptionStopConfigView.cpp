/*
 * Copyright 2013-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "ExceptionStopConfigView.h"

#include <CheckBox.h>
#include <LayoutBuilder.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "FunctionInstance.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "MessageCodes.h"
#include "UserInterface.h"
#include "Team.h"


enum {
	MSG_STOP_ON_THROWN_EXCEPTION_CHANGED	= 'stec',
	MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED	= 'scec',
};


ExceptionStopConfigView::ExceptionStopConfigView(::Team* team,
	UserInterfaceListener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fListener(listener),
	fExceptionThrown(NULL),
	fExceptionCaught(NULL)
{
	SetName("Exceptions");
}


ExceptionStopConfigView::~ExceptionStopConfigView()
{
}


ExceptionStopConfigView*
ExceptionStopConfigView::Create(::Team* team, UserInterfaceListener* listener)
{
	ExceptionStopConfigView* self = new ExceptionStopConfigView(
		team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}


void
ExceptionStopConfigView::AttachedToWindow()
{
	fExceptionThrown->SetTarget(this);
	fExceptionCaught->SetTarget(this);

	AutoLocker< ::Team> teamLocker(fTeam);
	_UpdateExceptionState();

	BGroupView::AttachedToWindow();
}


void
ExceptionStopConfigView::MessageReceived(BMessage* message)
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
			BGroupView::MessageReceived(message);
			break;
	}

}


void
ExceptionStopConfigView::_Init()
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGlue()
		.Add(fExceptionThrown = new BCheckBox("exceptionThrown",
			"Stop when an exception is thrown",
			new BMessage(MSG_STOP_ON_THROWN_EXCEPTION_CHANGED)))
		.Add(fExceptionCaught = new BCheckBox("exceptionCaught",
			"Stop when an exception is caught",
			new BMessage(MSG_STOP_ON_CAUGHT_EXCEPTION_CHANGED)))
		.AddGlue();

	// TODO: enable once implemented
	fExceptionCaught->SetEnabled(false);
}


void
ExceptionStopConfigView::_UpdateThrownBreakpoints(bool enable)
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
ExceptionStopConfigView::_FindExceptionFunction(ImageDebugInfo* info,
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


void
ExceptionStopConfigView::_UpdateExceptionState()
{
	// check if the exception breakpoints are already installed
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
