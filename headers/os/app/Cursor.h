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
//	File Name:		Cursor.h
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BCursor describes a view-wide or application-wide cursor.
//------------------------------------------------------------------------------

#ifndef _CURSOR_H
#define _CURSOR_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Archivable.h>
#include <BeBuild.h>
#include <InterfaceDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// BCursor class ---------------------------------------------------------------
class BCursor : BArchivable {
public:
							BCursor(const void* cursorData);
							BCursor(BMessage* data);
	virtual					~BCursor();

	virtual	status_t		Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*	Instantiate(BMessage* data);

// Private or reserved ---------------------------------------------------------
	virtual status_t		Perform(perform_code d, void* arg);

private:

	virtual	void			_ReservedCursor1();
	virtual	void			_ReservedCursor2();
	virtual	void			_ReservedCursor3();
	virtual	void			_ReservedCursor4();

	friend class			BApplication;
	friend class			BView;

	int32					m_serverToken;
	int32					m_needToFree;
	uint32					_reserved[6];
};
//------------------------------------------------------------------------------

#endif	// _CURSOR_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

