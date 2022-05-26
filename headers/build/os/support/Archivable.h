//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		Archivable.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BArchivable mix-in class defines the archiving
//					protocol.  Also some global archiving functions.
//------------------------------------------------------------------------------

#ifndef _ARCHIVABLE_H
#define _ARCHIVABLE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <image.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BMessage;

// BArchivable class -----------------------------------------------------------
class BArchivable {
public:
						BArchivable();
	virtual				~BArchivable();

						BArchivable(BMessage* from);
	virtual	status_t	Archive(BMessage* into, bool deep = true) const;
	static	BArchivable	*Instantiate(BMessage* from);

// Private or reserved ---------------------------------------------------------
	virtual status_t	Perform(perform_code d, void* arg);

private:

	virtual	void		_ReservedArchivable1();
	virtual	void		_ReservedArchivable2();
	virtual	void		_ReservedArchivable3();

			uint32		_reserved[2];
};


// Global Functions ------------------------------------------------------------

typedef BArchivable* (*instantiation_func) (BMessage *);

bool				validate_instantiation(BMessage* from,
												   const char* class_name);


//------------------------------------------------------------------------------


#endif	// _ARCHIVABLE_H
