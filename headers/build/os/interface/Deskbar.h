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
//	File Name:		Deskbar.h
//	Author:			Jeremy Rand (jrand@magma.ca)
//	Description:	BDeskbar allows one to control the deskbar from an
//                  application.
//------------------------------------------------------------------------------
 
#ifndef	_DESKBAR_H
#define	_DESKBAR_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Rect.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BMessenger;
class BView;

// enum for deskbar locations --------------------------------------------------
enum deskbar_location {
	B_DESKBAR_TOP,
	B_DESKBAR_BOTTOM,
	B_DESKBAR_LEFT_TOP,
	B_DESKBAR_RIGHT_TOP,
	B_DESKBAR_LEFT_BOTTOM,
	B_DESKBAR_RIGHT_BOTTOM
};


// BDeskbar class -------------------------------------------------------------
class BDeskbar
{
public:
	BDeskbar();
	~BDeskbar();
	
// Location member functions
	BRect Frame(void) const;
	deskbar_location Location(bool *isExpanded = NULL) const;
	status_t SetLocation(deskbar_location location, bool expanded = false);
	bool IsExpanded(void) const;
	status_t Expand(bool expand);
	
// Item querying member functions
	status_t GetItemInfo(int32 id, const char **name) const;
	status_t GetItemInfo(const char *name, int32 *id) const;
	bool HasItem(int32 id) const;
	bool HasItem(const char *name) const;
	uint32 CountItems(void) const;
	
// Item modification member functions
	status_t AddItem(BView *archivableView, int32 *id = NULL);
	status_t AddItem(entry_ref *addon, int32 *id = NULL);
	status_t RemoveItem(int32 id);
	status_t RemoveItem(const char *name);

private:
	BMessenger *fMessenger;
	uint32			_reserved[12];
};
//------------------------------------------------------------------------------

#endif	// _DESKBAR_H

