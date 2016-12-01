/*
 * Copyright 2015-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "SignalDispositionEditWindow.h"

#include <signal.h>

#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuField.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "AppMessageCodes.h"
#include "SignalDispositionMenu.h"
#include "SignalDispositionTypes.h"
#include "Team.h"
#include "UiUtils.h"
#include "UserInterface.h"


enum {
	MSG_SELECTED_SIGNAL_CHANGED 		= 'ssic',
	MSG_SELECTED_DISPOSITION_CHANGED 	= 'sdic',
	MSG_SAVE_SIGNAL_DISPOSITION 		= 'ssid'
};


SignalDispositionEditWindow::SignalDispositionEditWindow(::Team* team,
	int32 signal, UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Edit signal disposition", B_FLOATING_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fEditMode(signal > 0),
	fCurrentSignal(signal),
	fSaveButton(NULL),
	fCancelButton(NULL),
	fSignalSelectionField(NULL),
	fDispositionSelectionField(NULL),
	fTarget(target)
{
}


SignalDispositionEditWindow::~SignalDispositionEditWindow()
{
	BMessenger(fTarget).SendMessage(MSG_SIGNAL_DISPOSITION_EDIT_WINDOW_CLOSED);
}


SignalDispositionEditWindow*
SignalDispositionEditWindow::Create(::Team* team, int32 signal,
	UserInterfaceListener* listener, BHandler* target)
{
	SignalDispositionEditWindow* self = new SignalDispositionEditWindow(
		team, signal, listener, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;

}

void
SignalDispositionEditWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SELECTED_SIGNAL_CHANGED:
		{
			int32 signal;
			if (message->FindInt32("signal", &signal) == B_OK)
				fCurrentSignal = signal;
			break;
		}
		case MSG_SELECTED_DISPOSITION_CHANGED:
		{
			int32 disposition;
			if (message->FindInt32("disposition", &disposition) == B_OK)
				fCurrentDisposition = disposition;
			break;
		}
		case MSG_SAVE_SIGNAL_DISPOSITION:
		{
			fListener->SetCustomSignalDispositionRequested(fCurrentSignal,
				fCurrentDisposition);
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
SignalDispositionEditWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
SignalDispositionEditWindow::_Init()
{
	SignalDispositionMenu* menu = new SignalDispositionMenu("dispositionMenu",
		new BMessage(MSG_SELECTED_DISPOSITION_CHANGED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add((fSignalSelectionField = new BMenuField("Signal:",
				_BuildSignalSelectionMenu())))
			.Add((fDispositionSelectionField = new BMenuField("Disposition:",
				menu)))
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add((fSaveButton = new BButton("Save",
				new BMessage(MSG_SAVE_SIGNAL_DISPOSITION))))
			.Add((fCancelButton = new BButton("Cancel",
				new BMessage(B_CANCEL))))
		.End()
	.End();

	fSignalSelectionField->Menu()->SetLabelFromMarked(true);
	fSignalSelectionField->Menu()->SetTargetForItems(this);
	menu->SetLabelFromMarked(true);
	menu->SetTargetForItems(this);

	AutoLocker< ::Team> teamLocker(fTeam);
	_UpdateState();

	// if we're editing an existing row, don't allow changing the signal
	// selection
	if (fEditMode)
		fSignalSelectionField->SetEnabled(false);


}


BMenu*
SignalDispositionEditWindow::_BuildSignalSelectionMenu()
{
	BMenu* menu = new BMenu("signals");
	BMenuItem* item;

	#undef ADD_SIGNAL_MENU_ITEM
	#define ADD_SIGNAL_MENU_ITEM(x)										\
		menu->AddItem((item = new BMenuItem(#x, new BMessage(			\
			MSG_SELECTED_SIGNAL_CHANGED))));							\
		item->Message()->AddInt32("signal", x);

	ADD_SIGNAL_MENU_ITEM(SIGHUP)
	ADD_SIGNAL_MENU_ITEM(SIGINT)
	ADD_SIGNAL_MENU_ITEM(SIGQUIT)
	ADD_SIGNAL_MENU_ITEM(SIGILL)
	ADD_SIGNAL_MENU_ITEM(SIGCHLD)
	ADD_SIGNAL_MENU_ITEM(SIGABRT)
	ADD_SIGNAL_MENU_ITEM(SIGPIPE)
	ADD_SIGNAL_MENU_ITEM(SIGFPE)
	ADD_SIGNAL_MENU_ITEM(SIGKILL)
	ADD_SIGNAL_MENU_ITEM(SIGSTOP)
	ADD_SIGNAL_MENU_ITEM(SIGSEGV)
	ADD_SIGNAL_MENU_ITEM(SIGCONT)
	ADD_SIGNAL_MENU_ITEM(SIGTSTP)
	ADD_SIGNAL_MENU_ITEM(SIGALRM)
	ADD_SIGNAL_MENU_ITEM(SIGTERM)
	ADD_SIGNAL_MENU_ITEM(SIGTTIN)
	ADD_SIGNAL_MENU_ITEM(SIGTTOU)
	ADD_SIGNAL_MENU_ITEM(SIGUSR1)
	ADD_SIGNAL_MENU_ITEM(SIGUSR2)
	ADD_SIGNAL_MENU_ITEM(SIGWINCH)
	ADD_SIGNAL_MENU_ITEM(SIGKILLTHR)
	ADD_SIGNAL_MENU_ITEM(SIGTRAP)
	ADD_SIGNAL_MENU_ITEM(SIGPOLL)
	ADD_SIGNAL_MENU_ITEM(SIGPROF)
	ADD_SIGNAL_MENU_ITEM(SIGSYS)
	ADD_SIGNAL_MENU_ITEM(SIGURG)
	ADD_SIGNAL_MENU_ITEM(SIGVTALRM)
	ADD_SIGNAL_MENU_ITEM(SIGXCPU)
	ADD_SIGNAL_MENU_ITEM(SIGXFSZ)
	ADD_SIGNAL_MENU_ITEM(SIGBUS)

	BString signalName;
	for (int32 i = SIGRTMIN; i <= SIGRTMAX; i++) {
		menu->AddItem((item = new BMenuItem(UiUtils::SignalNameToString(i,
					signalName), new BMessage(MSG_SELECTED_SIGNAL_CHANGED))));
		item->Message()->AddInt32("signal", i);
	}

	return menu;
}


void
SignalDispositionEditWindow::_UpdateState()
{
	if (fCurrentSignal <= 0)
		fCurrentSignal = SIGHUP;

	fSignalSelectionField->Menu()->ItemAt(fCurrentSignal - 1)->SetMarked(true);

	fCurrentDisposition = fTeam->SignalDispositionFor(fCurrentSignal);
	fDispositionSelectionField->Menu()->ItemAt(fCurrentDisposition)->SetMarked(
		true);
}

