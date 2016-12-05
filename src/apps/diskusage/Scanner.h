/*
 * Copyright (c) 2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT/X11 license.
 *
 * Copyright (c) 1999 Mike Steed. You are free to use and distribute this software
 * as long as it is accompanied by it's documentation and this copyright notice.
 * The software comes with no warranty, etc.
 */
#ifndef SCANNER_H
#define SCANNER_H


#include <string>

#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <Volume.h>

#include "Snapshot.h"


class BDirectory;

using std::string;


class Scanner: public BLooper {
public:
								Scanner(BVolume* volume, BHandler* handler);
	virtual						~Scanner();

	virtual	void				MessageReceived(BMessage* message);

			VolumeSnapshot*		Snapshot() const
									{ return fBusy ? NULL : fSnapshot; }
			void				Refresh(FileInfo* startInfo = NULL);
			void				Cancel();
			bool				IsBusy() const
									{ return fBusy; }
			const char*			Task() const
									{ return fTask.c_str(); }
			float				Progress() const
									{ return min_c(1.0, fProgress); }
			FileInfo*			CurrentDir() const
									{ return fBusy ? NULL : fSnapshot->currentDir; }
			void				ChangeDir(FileInfo* info)
									{ fSnapshot->currentDir = info; }
			void				SetDesiredPath(string &path);
			dev_t				Device() const
									{ return fVolume->Device(); }
			void				RequestQuit();

private:
			void				_RunScan(FileInfo *startInfo);
			FileInfo*			_GetFileInfo(BDirectory* dir, FileInfo* parent);
			void				_ChangeToDesired();
			bool				_DirectoryContains(FileInfo* currentDir,
									entry_ref* ref);

			BMessenger			fListener;
			BMessage			fDoneMessage;
			BMessage			fProgressMessage;

			BVolume*			fVolume;
			off_t				fVolumeBytesInUse;
			off_t				fVolumeBytesScanned;
			float				fProgress;
			float				fLastReport;
			VolumeSnapshot*		fSnapshot;
			string				fDesiredPath;
			string				fTask;
			bool				fBusy;
			bool				fQuitRequested;
};

#endif // SCANNER_H
