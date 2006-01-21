/*
 * Copyright 2005, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#ifndef _INSTALLERCOPYLOOPCONTROL_H
#define _INSTALLERCOPYLOOPCONTROL_H

#include "FSUndoRedo.h"
#include "FSUtils.h"
class InstallerWindow;

class InstallerCopyLoopControl : public CopyLoopControl
{
public:
		InstallerCopyLoopControl(InstallerWindow *window);
		virtual ~InstallerCopyLoopControl() {};
		
		virtual bool FileError(const char *message, const char *name, status_t error,
			bool allowContinue);

		virtual void UpdateStatus(const char *name, entry_ref ref, int32 count, 
			bool optional = false);

		virtual bool CheckUserCanceled();

		virtual OverwriteMode OverwriteOnConflict(const BEntry *srcEntry, 
			const char *destName, const BDirectory *destDir, bool srcIsDir, 
			bool dstIsDir);

		virtual bool SkipEntry(const BEntry *, bool file);
		
		virtual void ChecksumChunk(const char *block, size_t size);
		virtual bool ChecksumFile(const entry_ref *);
		virtual bool SkipAttribute(const char *attributeName);
		virtual bool PreserveAttribute(const char *attributeName);
	
private:
	InstallerWindow *fWindow;
	BMessenger fMessenger;
};

#endif
