/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#include "InstallerCopyLoopControl.h"

/* copied from libtracker, because beos loader doesn't seem to find them correctly in libtracker.so */

/*BPrivate::CopyLoopControl::~CopyLoopControl()
{
}

void 
BPrivate::CopyLoopControl::ChecksumChunk(const char *, size_t)
{
}


bool 
BPrivate::CopyLoopControl::ChecksumFile(const entry_ref *)
{
	return true;
}


bool
BPrivate::CopyLoopControl::SkipAttribute(const char*)
{
	return false;
}


bool
BPrivate::CopyLoopControl::PreserveAttribute(const char*)
{
	return false;
}*/

InstallerCopyLoopControl::InstallerCopyLoopControl(InstallerWindow *window)
	: fWindow(window)
{
}


bool 
InstallerCopyLoopControl::FileError(const char *message, const char *name, status_t error,
			bool allowContinue)
{
	return false;
}
	

void 
InstallerCopyLoopControl::UpdateStatus(const char *name, entry_ref ref, int32 count, 
			bool optional)
{
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

