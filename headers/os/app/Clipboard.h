/*
 * Copyright 2007-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Gabe Yoder, gyoder@stny.rr.com
 */
#ifndef _CLIPBOARD_H
#define	_CLIPBOARD_H


#include <BeBuild.h>
#include <Locker.h>
#include <Messenger.h>

class BMessage;


enum {
	B_CLIPBOARD_CHANGED = 'CLCH'
};

class BClipboard {
public:
								BClipboard(const char* name,
									bool transient = false);
	virtual						~BClipboard();

			const char*			Name() const;

			uint32				LocalCount() const;
			uint32				SystemCount() const;
			status_t			StartWatching(BMessenger target);
			status_t			StopWatching(BMessenger target);

			bool				Lock();
			void				Unlock();
			bool				IsLocked() const;

			status_t			Clear();
			status_t			Commit();
			status_t			Commit(bool failIfChanged);
			status_t			Revert();

			BMessenger			DataSource() const;
			BMessage*			Data() const;

private:
								BClipboard(const BClipboard&);
			BClipboard&			operator=(const BClipboard&);

	virtual	void				_ReservedClipboard1();
	virtual	void				_ReservedClipboard2();
	virtual	void				_ReservedClipboard3();

			bool				_AssertLocked() const;
			status_t			_DownloadFromSystem(bool force = false);
			status_t			_UploadToSystem();
	
			uint32				_reserved0;
			BMessage*			fData;
			BLocker				fLock;
			BMessenger			fClipHandler;
			BMessenger			fDataSource;
			uint32				fCount;
			char*				fName;
			uint32				_reserved[4];
};

extern BClipboard* be_clipboard;

#endif	// _CLIPBOARD_H
