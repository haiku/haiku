/******************************************************************************
/
/	File:			Clipboard.h
/
/	Description:	BClipboard class defines clipboard functionality.
/					The global be_clipboard represents the default clipboard.
/
/	Copyright 1995-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#ifndef _CLIPBOARD_H
#define	_CLIPBOARD_H

#include <BeBuild.h>
#include <Messenger.h>
#include <Locker.h>

class BMessage;

enum {
	B_CLIPBOARD_CHANGED = 'CLCH'
};

/*------------------------------------------------------------------*/
/*----- BClipboard class --------------------------------------------*/

class BClipboard {
public:
					BClipboard(const char *name, bool transient = false);
virtual				~BClipboard();

		const char	*Name() const;

		uint32		LocalCount() const;
		uint32		SystemCount() const;
		status_t	StartWatching(BMessenger target);
		status_t	StopWatching(BMessenger target);

		bool		Lock();
		void		Unlock();
		bool		IsLocked() const;
		
		status_t	Clear();
		status_t	Commit();
		status_t	Revert();

		BMessenger	DataSource() const;
		BMessage	*Data() const;

/*----- Private or reserved -----------------------------------------*/
private:
					BClipboard(const BClipboard &);
		BClipboard	&operator=(const BClipboard &);

virtual	void		_ReservedClipboard1();
virtual	void		_ReservedClipboard2();
virtual	void		_ReservedClipboard3();

		bool		AssertLocked() const;
		status_t	DownloadFromSystem(bool force = false);
		status_t	UploadToSystem();

		uint32		fCount;
		BMessage	*fData;
		BLocker		fLock;
		BMessenger	fClipHandler;
		BMessenger	fDataSource;
		uint32		fSystemCount;
		char		*fName;
		uint32		_reserved[4];
};

/*----------------------------------------------------------------*/
/*----- Global Clipboard -----------------------------------------*/

extern _IMPEXP_BE BClipboard *be_clipboard;

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _CLIPBOARD_H */
