/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Alert.h>
#include <Messenger.h>
#include <Roster.h>
#include <View.h>

#include "PoseView.h"

static void Error(BView *view, status_t status, bool unlock=false)
{
	BAlert *alert;
	if (view && unlock)
		view->UnlockLooper();
	BString s(strerror(status));
	alert = new BAlert("Error", s.String(), "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}

/*!
	\brief Tracker add-on entry
*/
extern "C" void
process_refs(entry_ref dir, BMessage* refs, void* /*reserved*/)
{
	status_t status;
	BAlert *alert;
	BMessenger msgr;
	BPoseView *view = NULL;
	BMessage poseViewBackup;
	BMessage poseViewColumnBackup;
	uint32 poseViewModeBackup;
	BString windowTitleBackup;

	refs->PrintToStream();
	
	status = refs->FindMessenger("TrackerViewToken", &msgr);
	if (status < B_OK) {
		Error(view, status);
		return;
	}

	status = B_ERROR;
	if (!msgr.LockTarget()) {
		Error(view, status);
		return;
	}

	status = B_BAD_HANDLER;
	view = dynamic_cast<BPoseView *>(msgr.Target(NULL));
	if (!view) {
		Error(view, status);
		return;
	}
	if (dynamic_cast<BWindow *>(view->Looper()) == NULL) {
		Error(view, status, true);
		return;
	}

	windowTitleBackup = view->Window()->Title();

	view->SaveColumnState(poseViewColumnBackup);
	view->SaveState(poseViewBackup);
	view->SetDragEnabled(false);
	view->SetSelectionRectEnabled(false);
	view->SetPoseEditing(false);
	poseViewModeBackup = view->ViewMode();
	

	view->SetViewMode(kIconMode);

	view->ShowBarberPole();


	view->UnlockLooper();



	alert = new BAlert("Error", "IconVader:\nClick on the icons to get points."
		"\nAvoid symlinks!", "OK");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();


	int32 score = 0;
	int32 count = 300;
	while (count--) {
		status = B_ERROR;
		if (!msgr.LockTarget()) {
			Error(view, status);
			return;
		}

		BPose *pose;
		for (int32 i = 0; (pose = view->PoseAtIndex(i)); i++) {
			if (pose->IsSelected()) {
				if (pose->TargetModel()->IsFile())
					score++;
				if (pose->TargetModel()->IsDirectory())
					score+=2;
				if (pose->TargetModel()->IsSymLink())
					score-=10;
				pose->Select(false);
			}
#ifdef __HAIKU__
			BPoint location = pose->Location(view);
#else
			BPoint location = pose->Location();
#endif
			location.x += ((rand() % 20) - 10);
			location.y += ((rand() % 20) - 10);
#ifdef __HAIKU__
			pose->SetLocation(location, view);
#else
			pose->SetLocation(location);
#endif
		}

		view->CheckPoseVisibility();

		view->Invalidate();

		BString str("Score: ");
		str << score;
		view->Window()->SetTitle(str.String());
		
		view->UnlockLooper();
		snooze(100000);
	}

	BString scoreStr("You scored ");
	scoreStr << score << " points!";
	alert = new BAlert("Error", scoreStr.String(), "Cool!");
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();


	status = B_ERROR;
	if (!msgr.LockTarget()) {
		Error(view, status);
		return;
	}

	/*
	status = B_BAD_HANDLER;
	view = dynamic_cast<BPoseView *>(msgr.Target(NULL));
	if (!view)
		goto err1;
	*/

	view->HideBarberPole();
	view->SetViewMode(poseViewModeBackup);
	view->RestoreState(poseViewBackup);
	view->RestoreColumnState(poseViewColumnBackup);

	view->Window()->SetTitle(windowTitleBackup.String());
	
/*
	BMessage('_RRC') {
        TrackerViewToken = BMessenger(port=32004, team=591, target=direct:0x131)
} */


	//be_roster->Launch("application/x-vnd.haiku-filetypes", refs);
	
	view->UnlockLooper();
	return;

}
