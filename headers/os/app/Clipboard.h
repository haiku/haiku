//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Clipboard.h
//	Author:			Gabe Yoder (gyoder@stny.rr.com)
//	Description:	BClipboard provides an interface to a system-wide clipboard
//                  storage area.
//------------------------------------------------------------------------------

#ifndef _CLIPBOARD_H
#define	_CLIPBOARD_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Messenger.h>
#include <Locker.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
class BMessage;

enum {
	B_CLIPBOARD_CHANGED = 'CLCH'
};

// BClipboard class ---------------------------------------------------------------
class BClipboard {
public:
				BClipboard(const char *name, bool transient = false);
		virtual		~BClipboard();

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

		virtual	void	_ReservedClipboard1();
		virtual	void	_ReservedClipboard2();
		virtual	void	_ReservedClipboard3();

		bool		AssertLocked() const;
		status_t	DownloadFromSystem(bool force = false);
		status_t	UploadToSystem();

		uint32		_reserved0;
		BMessage	*fData;
		BLocker		fLock;
		BMessenger	fClipHandler;
		BMessenger	fDataSource;
		uint32		fCount;
		char		*fName;
		uint32		_reserved[4];
};

//----- Global Clipboard -------------------------------------------------------

extern _IMPEXP_BE BClipboard *be_clipboard;

//------------------------------------------------------------------------------

#endif	// _CLIPBOARD_H

