/*

"Backgrounds" by Jerome Duval (jerome.duval@free.fr)

(C)2002 OpenBeOS under MIT license

*/

#include "BGWindow.h"

BGWindow::BGWindow(BRect frame, bool standalone)
	: BWindow(frame, WINDOW_TITLE, B_TITLED_WINDOW, 
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE, B_ALL_WORKSPACES),
		fIsStandalone(standalone)
{
	fBGView = new BGView(Bounds(),"BackgroundsView",B_FOLLOW_ALL, B_WILL_DRAW);
	AddChild(fBGView);
}


bool
BGWindow::QuitRequested()
{
	fBGView->SaveSettings();
	if(fIsStandalone)
		be_app->PostMessage(B_QUIT_REQUESTED);
	return(true);
}


void
BGWindow::WorkspaceActivated(int32 oldWorkspaces, bool active)
{
	fBGView->WorkspaceActivated(oldWorkspaces, active);
}


void 
BGWindow::ProcessRefs(entry_ref dir_ref, BMessage* msg)
{
	fBGView->ProcessRefs(dir_ref, msg);
}
