/*
 * Copyright 2005-2006, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"
#include "InstallerWindow.h"
#include "PartitionMenuItem.h"
#include <Alert.h>
#include <FindDirectory.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>

//#define COPY_TRACE
#ifdef COPY_TRACE
#define CALLED() 			printf("CALLED %s\n",__PRETTY_FUNCTION__)
#else
#define CALLED()
#endif

const char BOOT_PATH[] = "/boot";

extern void SizeAsString(off_t size, char *string);


CopyEngine::CopyEngine(InstallerWindow *window)
	: BLooper("copy_engine"),
	fWindow(window),
	fPackages(NULL),
	fSpaceRequired(0)
{
	Run();
}


void
CopyEngine::MessageReceived(BMessage*msg)
{
	CALLED();
	switch (msg->what) {
		case ENGINE_START:
			Start(fWindow->GetSourceMenu(), fWindow->GetTargetMenu());
			break;
	}
}


void
CopyEngine::SetStatusMessage(char *status)
{
	BMessage msg(STATUS_MESSAGE);
	msg.AddString("status", status);
	BMessenger(fWindow).SendMessage(&msg);
}


void
CopyEngine::Start(BMenu *srcMenu, BMenu *targetMenu)
{
	CALLED();
	PartitionMenuItem *targetItem = (PartitionMenuItem *)targetMenu->FindMarked();
	PartitionMenuItem *srcItem = (PartitionMenuItem *)srcMenu->FindMarked();
	if (!srcItem || !targetItem) {
		fprintf(stderr, "bad menu items\n");
		return;
	}
	
	BMessage msg(INSTALL_FINISHED);
	BMessenger(fWindow).SendMessage(&msg);
}


void
CopyEngine::ScanDisksPartitions(BMenu *srcMenu, BMenu *targetMenu)
{
	PartitionMenuItem *item = new PartitionMenuItem(NULL, "boot", NULL, new BMessage(SRC_PARTITION), 0);
	srcMenu->AddItem(item);

	PartitionMenuItem *item2 = new PartitionMenuItem(NULL, "target", NULL, new BMessage(TARGET_PARTITION), 0);
	targetMenu->AddItem(item2);
}


void
CopyEngine::SetPackagesList(BList *list)
{
	if (fPackages)
		delete fPackages;
	fPackages = list;
}

