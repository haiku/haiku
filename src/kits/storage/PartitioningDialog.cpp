//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskScannerAddOn.h>
#include <Screen.h>

#include "PartitioningDialog.h"

// constructor
PartitioningDialog::PartitioningDialog(BRect dialogCenter)
	: BWindow(BRect(0, 0, 1, 1), "Partitioning Parameters", B_TITLED_WINDOW,
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
	if (fBlocker >= 0)
		delete_sem(fBlocker);
}

// Go
status_t
PartitioningDialog::Go(BDiskScannerParameterEditor *editor, bool *_cancelled)
{
	status_t error = (editor ? B_OK : B_BAD_VALUE);
	// set the parameter editor and view
	if (error == B_OK) {
		fEditor = editor;
		fEditorView = editor->View();
		if (!fEditorView)
			error = B_ERROR;
	}
	bool wasRun = false;
	// fire up the window
	if (error == B_OK) {
		if (fBlocker >= 0) {
			sem_id blocker = fBlocker;
			bool cancelled = true;
			fCancelled = &cancelled;
			Run();
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

