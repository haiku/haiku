/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#include "InstallerCopyLoopControl.h"
#include "InstallerWindow.h"
#include <Alert.h>

InstallerCopyLoopControl::InstallerCopyLoopControl(InstallerWindow *window)
	: fWindow(window),
	fMessenger(window)
{
}


bool 
InstallerCopyLoopControl::FileError(const char *message, const char *name, status_t error,
			bool allowContinue)
{
	char buffer[512];
	sprintf(buffer, message, name, strerror(error));

	if (allowContinue)
		return (new BAlert("", buffer, "Cancel", "OK", 0,
			B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go() != 0;

	(new BAlert("", buffer, "Cancel", 0, 0,
		B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
	return false;
}
	

void 
InstallerCopyLoopControl::UpdateStatus(const char *name, entry_ref ref, int32 count, 
			bool optional)
{
	if (name) {
		BMessage msg(STATUS_MESSAGE);
		BString s = "Installing ";
		s << name;
		msg.AddString("status", s.String());
		fMessenger.SendMessage(&msg);
	}
}


bool 
InstallerCopyLoopControl::CheckUserCanceled()
{
	return false;
}


InstallerCopyLoopControl::OverwriteMode 
InstallerCopyLoopControl::OverwriteOnConflict(const BEntry *srcEntry, 
			const char *destName, const BDirectory *destDir, bool srcIsDir, 
			bool dstIsDir)
{
	if (srcIsDir && dstIsDir)
		return kMerge;
	else
		return kReplace;
}


bool 
InstallerCopyLoopControl::SkipEntry(const BEntry *, bool file)
{
	return false;
}


void 
InstallerCopyLoopControl::ChecksumChunk(const char *block, size_t size)
{
}


bool 
InstallerCopyLoopControl::ChecksumFile(const entry_ref *ref)
{
	return true;
}


bool 
InstallerCopyLoopControl::SkipAttribute(const char *attributeName)
{
	return false;
}


bool 
InstallerCopyLoopControl::PreserveAttribute(const char *attributeName)
{
	return false;
}

