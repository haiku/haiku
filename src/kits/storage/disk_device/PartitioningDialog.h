//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------

#ifndef _PARTITIONING_DIALOG_H
#define _PARTITIONING_DIALOG_H

#include <OS.h>
#include <Window.h>

class BDiskScannerParameterEditor;

namespace BPrivate {

class PartitioningDialog : public BWindow {
public:
	PartitioningDialog(BRect dialogCenter);
	virtual ~PartitioningDialog();

	virtual void MessageReceived(BMessage *message);
	virtual bool QuitRequested();

	status_t Go(BDiskScannerParameterEditor *editor, bool *cancelled);

private:
	status_t _Init(BDiskScannerParameterEditor *editor);

private:
	BDiskScannerParameterEditor	*fEditor;
	BView						*fEditorView;
	sem_id						fBlocker;
	bool						*fCancelled;
	BPoint						fCenter;
};

}	// namespace BPrivate

using BPrivate::PartitioningDialog;

#endif	// _PARTITIONING_DIALOG_H
