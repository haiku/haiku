/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "ApplicationsView.h"

#include "LaunchButton.h"
#include "Switcher.h"


static const uint32 kMsgActivateApp = 'AcAp';


ApplicationsView::ApplicationsView(uint32 location)
	:
	BGroupView((location & (kTopEdge | kBottomEdge)) != 0
		? B_HORIZONTAL : B_VERTICAL)
{
}


ApplicationsView::~ApplicationsView()
{
}


void
ApplicationsView::AttachedToWindow()
{
	// TODO: make this dynamic!

	BList teamList;
	be_roster->GetAppList(&teamList);

	for (int32 i = 0; i < teamList.CountItems(); i++) {
		app_info appInfo;
		team_id team = (uintptr_t)teamList.ItemAt(i);
		if (be_roster->GetRunningAppInfo(team, &appInfo) == B_OK)
			_AddTeam(appInfo);
	}
}


void
ApplicationsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgActivateApp:
			be_roster->ActivateApp(message->FindInt32("team"));
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ApplicationsView::_AddTeam(app_info& info)
{
	if ((info.flags & B_BACKGROUND_APP) != 0)
		return;

	BMessage* message = new BMessage(kMsgActivateApp);
	message->AddInt32("team", info.team);

	LaunchButton* button = new LaunchButton(info.signature, NULL, message,
		this);
	button->SetTo(&info.ref);
	AddChild(button);
}
