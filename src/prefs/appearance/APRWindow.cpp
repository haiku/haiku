#include <Messenger.h>
#include "APRWindow.h"
#include "APRView.h"
#include "DecView.h"
#include "CurView.h"
#include "defs.h"

APRWindow::APRWindow(BRect frame)
	: BWindow(frame, "Appearance", B_TITLED_WINDOW,
	  B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES )
{
	tabview=new BTabView(Bounds(),"TabView");

	BTab *tab=NULL;

	colors=new APRView(Bounds(),"Colors",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(colors);
	tabview->AddTab(colors,tab);

	decorators=new DecView(Bounds(),"Decorator",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(decorators);
	tabview->AddTab(decorators,tab);

	cursors=new CurView(Bounds(),"Cursors",B_FOLLOW_ALL, B_WILL_DRAW);
	tab=new BTab(cursors);
	tabview->AddTab(cursors,tab);

	AddChild(tabview);
	decorators->SetColors(colors->settings);
}

bool APRWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}

void APRWindow::WorkspaceActivated(int32 wkspc, bool is_active)
{
	if(is_active)
	{
		BMessenger notifier(colors);
		notifier.SendMessage(new BMessage(B_WORKSPACE_ACTIVATED));
	}
}

#include <stdio.h>
void APRWindow::MessageReceived(BMessage *msg)
{
	if(msg->what==SET_UI_COLORS)
		decorators->SetColors(colors->settings);
	else
		BWindow::MessageReceived(msg);
}
