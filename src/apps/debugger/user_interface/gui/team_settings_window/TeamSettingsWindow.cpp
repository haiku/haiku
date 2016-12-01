/*
 * Copyright 2013-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#include "TeamSettingsWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <TabView.h>

#include "AppMessageCodes.h"
#include "ExceptionStopConfigView.h"
#include "ImageStopConfigView.h"
#include "SignalsConfigView.h"


TeamSettingsWindow::TeamSettingsWindow(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
	:
	BWindow(BRect(), "Team settings", B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fTeam(team),
	fListener(listener),
	fCloseButton(NULL),
	fTarget(target)
{
}


TeamSettingsWindow::~TeamSettingsWindow()
{
	BMessenger(fTarget).SendMessage(MSG_TEAM_SETTINGS_WINDOW_CLOSED);
}


TeamSettingsWindow*
TeamSettingsWindow::Create(::Team* team,
	UserInterfaceListener* listener, BHandler* target)
{
	TeamSettingsWindow* self = new TeamSettingsWindow(
		team, listener, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
TeamSettingsWindow::Show()
{
	CenterOnScreen();
	BWindow::Show();
}


void
TeamSettingsWindow::_Init()
{
	BTabView* tabView = new BTabView("config tab view");
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(tabView)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fCloseButton = new BButton("Close", new BMessage(
					B_QUIT_REQUESTED)))
		.End();

	SignalsConfigView* signalsView = SignalsConfigView::Create(fTeam,
		fListener);
	tabView->AddTab(signalsView);
	ImageStopConfigView* imageView = ImageStopConfigView::Create(fTeam,
		fListener);
	tabView->AddTab(imageView);
	ExceptionStopConfigView* exceptionView = ExceptionStopConfigView::Create(
		fTeam, fListener);
	tabView->AddTab(exceptionView);

	fCloseButton->SetTarget(this);
}
