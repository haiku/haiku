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
//	File Name:		Cursor.cpp
//	Author:			Frans van Nispen (xlr8@tref.nl)
//	Description:	BCursor describes a view-wide or application-wide cursor.
//------------------------------------------------------------------------------
/**
	@note:	As BeOS only supports 16x16 monochrome cursors, and I would like
			to see a nice shadowes one, we will need to extend this one.
 */

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Cursor.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
BCursor::BCursor(const void *cursorData)
{
//	uint8 data[68];
//	memcpy(&data, cursorData, 68);
}
//------------------------------------------------------------------------------
// undefined on BeOS
BCursor::BCursor(BMessage *data)
{
}
//------------------------------------------------------------------------------
BCursor::~BCursor()
{
}
//------------------------------------------------------------------------------
// not implemented on BeOS
status_t BCursor::Archive(BMessage *into, bool deep = true) const
{
	return B_OK;
}
//------------------------------------------------------------------------------
// not implemented on BeOS
BArchivable	*BCursor::Instantiate(BMessage *data)
{
	return NULL;
}
//------------------------------------------------------------------------------
status_t BCursor::Perform(perform_code d, void *arg)
{
	printf("perform %d\n", (int)d);
	return B_OK;
}
//------------------------------------------------------------------------------
void BCursor::_ReservedCursor1()
{
}
//------------------------------------------------------------------------------
void BCursor::_ReservedCursor2()
{
}
//------------------------------------------------------------------------------
void BCursor::_ReservedCursor3()
{
}
//------------------------------------------------------------------------------
void BCursor::_ReservedCursor4()
{
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

