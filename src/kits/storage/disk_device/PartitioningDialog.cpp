//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <algobase.h>
#include <new.h>
#include <stdio.h>
#include <string.h>

#include <Button.h>
#include <DiskScannerAddOn.h>
#include <Screen.h>
#include <View.h>

#include "PartitioningDialog.h"

enum {
	MSG_OK		= 'okay',
	MSG_CANCEL	= 'cncl',
};

// constructor
PartitioningDialog::PartitioningDialog(BRect dialogCenter)
	: BWindow(BRect(100, 100, 100, 100), "Partitioning parameters",
			  B_TITLED_WINDOW,
			  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE),
	  fEditor(NULL),
	  fEditorView(NULL),
	  fBlocker(-1),
	  fCancelled(NULL),
	  fCenter()
{
	fBlocker = create_sem(0, "partitioning dialog blocker");
	// calculate the dialog center point
	if (!dialogCenter.IsValid())
		dialogCenter = BScreen().Frame();
	fCenter = dialogCenter.LeftTop() + dialogCenter.RightBottom();
	fCenter.x /= 2;
	fCenter.y /= 2;
}

// destructor
PartitioningDialog::~PartitioningDialog()
{
	if (fEditorView)
		fEditorView->RemoveSelf();
	if (fBlocker >= 0)
		delete_sem(fBlocker);
}

// MessageReceived
void
PartitioningDialog::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_OK:
		{
			if (fEditor->EditingDone()) {
				if (fCancelled)
					*fCancelled = false;
				Quit();
			}
			break;
		}
		case MSG_CANCEL:
			if (fCancelled)
				*fCancelled = true;
			Quit();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
PartitioningDialog::QuitRequested()
{
	if (fCancelled)
		*fCancelled = true;
	return true;
}

// Go
status_t
PartitioningDialog::Go(BDiskScannerParameterEditor *editor, bool *_cancelled)
{
	status_t error = _Init(editor);
	bool wasRun = false;
	// fire up the window
	if (error == B_OK) {
		if (fBlocker >= 0) {
			sem_id blocker = fBlocker;
			bool cancelled = true;
			fCancelled = &cancelled;
			Show();
			wasRun = true;
			acquire_sem(blocker);
			if (cancelled)
				error = B_ERROR;
			if (_cancelled)
				*_cancelled = cancelled;
		} else
			error = fBlocker;
	}
	// delete the window if not run
	if (!wasRun)
		delete this;
	return error;
}

// _Init
status_t
PartitioningDialog::_Init(BDiskScannerParameterEditor *editor)
{
	status_t error = (editor ? B_OK : B_BAD_VALUE);
	// set the parameter editor and view
	if (error == B_OK) {
		fEditor = editor;
		fEditorView = editor->View();
		if (!fEditorView)
			error = B_ERROR;
	}
	// setup views
	if (error == B_OK) {
		BView *mainView = new(nothrow) BView(BRect(0, 0, 1, 1), "main",
											 B_FOLLOW_ALL, 0);
		BMessage *okMessage = new BMessage(MSG_OK);
		BMessage *cancelMessage = new BMessage(MSG_CANCEL);
		BButton *okButton = new (nothrow) BButton(
			BRect(0, 0, 1, 1), "ok", "OK", okMessage,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		BButton *cancelButton = new (nothrow) BButton(
			BRect(0, 0, 1, 1), "cancel", "Cancel", cancelMessage,
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
		if (okMessage && cancelMessage && okButton && cancelButton) {
			// add the views to the hierarchy
			mainView->AddChild(fEditorView);
			mainView->AddChild(okButton);
			mainView->AddChild(cancelButton);
			AddChild(mainView);
			// set the buttons' preferred size
			float buttonWidth = 0;
			float buttonHeight = 0;
			okButton->GetPreferredSize(&buttonWidth, &buttonHeight);
			okButton->ResizeTo(buttonWidth, buttonHeight);
			cancelButton->GetPreferredSize(&buttonWidth, &buttonHeight);
			cancelButton->ResizeTo(buttonWidth, buttonHeight);
			// setup their positions and sizes
			int32 hSpacing = 10;
			int32 vSpacing = 10;
			BRect editorRect = fEditorView->Bounds();
			BRect okRect = okButton->Bounds();
			BRect cancelRect = cancelButton->Bounds();
			// compute width and size of the main view
			int32 width = max(editorRect.IntegerWidth() + 1,
							  okRect.IntegerWidth() + 1 + hSpacing
							  + cancelRect.IntegerWidth() + 1)
						  + 2 * hSpacing;
			int32 height = editorRect.IntegerHeight() + 1
						   + max(okRect.IntegerHeight(),
						   		 cancelRect.IntegerHeight()) + 1
						   + 3 * vSpacing;
			mainView->ResizeTo(width - 1, height -1);
			ResizeTo(width - 1, height -1);
			// set location of the editor view
			fEditorView->MoveTo((width - editorRect.IntegerWidth() - 1) / 2,
								vSpacing - 1);
			fEditorView->ResizeTo(editorRect.Width(), editorRect.Height());
			// set the location of the buttons
			float buttonTop = 2 * vSpacing + editorRect.IntegerHeight();
			float buttonLeft = width - hSpacing - okRect.IntegerWidth();
			okButton->MoveTo(buttonLeft, buttonTop);
			buttonLeft -= hSpacing + cancelRect.IntegerWidth();
			cancelButton->MoveTo(buttonLeft, buttonTop);
		} else {
			// cleanup on error
			error = B_NO_MEMORY;
			if (!okButton && okMessage)
				delete okMessage;
			if (!cancelButton && cancelMessage)
				delete cancelMessage;
			delete okButton;
			delete cancelButton;
			delete mainView;
		}
	}
	return error;
}

