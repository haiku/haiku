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
//	File Name:		Clipboard.cpp
//	Author:			Gabe Yoder (gyoder@stny.rr.com)
//	Description:	BClipboard provides an interface to a system-wide clipboard
//                  storage area.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <Clipboard.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
BClipboard::BClipboard(const char *name, bool transient = false)
{
  /* Not complete yet */
  fName = strdup(name);
  fData = new BMessage();
}
//------------------------------------------------------------------------------
BClipboard::~BClipboard()
{
  /* Not complete yet? */
  free(fName);
  delete fData;
}
//------------------------------------------------------------------------------
const char* BClipboard::Name() const
{
  return (const char*)fName;
}
//------------------------------------------------------------------------------
uint32 BClipboard::LocalCount() const
{
  return fCount;
}
//------------------------------------------------------------------------------
uint32 BClipboard::SystemCount() const
{
  /* Need to upload count data from app server, and update fCount */

  return fCount;
}
//------------------------------------------------------------------------------
status_t BClipboard::StartWatching(BMessenger target)
{
}
//------------------------------------------------------------------------------
status_t BClipboard::StopWatching(BMessenger target)
{
}
//------------------------------------------------------------------------------
bool BClipboard::Lock()
{
  bool retVal;
  retVal = fLock.Lock();
  /* Still need to upload clipboard data to fData */
  /* Involves something with DownloadFromSystem */

  return retVal;
}
//------------------------------------------------------------------------------
void BClipboard::Unlock()
{
  fLock.Unlock();
}
//------------------------------------------------------------------------------
bool BClipboard::IsLocked() const
{
  return fLock.IsLocked();
}
//------------------------------------------------------------------------------
status_t BClipboard::Clear()
{
}
//------------------------------------------------------------------------------
status_t BClipboard::Commit()
{
}
//------------------------------------------------------------------------------
status_t BClipboard::Revert()
{
}
//------------------------------------------------------------------------------
BMessenger BClipboard::DataSource() const
{
}
//------------------------------------------------------------------------------
BMessage* BClipboard::Data() const
{
  if ( IsLocked() )
    return fData;
  return NULL;
}
//------------------------------------------------------------------------------
BClipboard::BClipboard(const BClipboard &)
{
}
//------------------------------------------------------------------------------
BClipboard & BClipboard::operator=(const BClipboard &)
{
}
//------------------------------------------------------------------------------
void BClipboard::_ReservedClipboard1()
{
}
//------------------------------------------------------------------------------
void BClipboard::_ReservedClipboard2()
{
}
//------------------------------------------------------------------------------
void BClipboard::_ReservedClipboard3()
{
}
//------------------------------------------------------------------------------
bool BClipboard::AssertLocked() const
{
}
//------------------------------------------------------------------------------
status_t BClipboard::DownloadFromSystem(bool force=false)
{
}
//------------------------------------------------------------------------------
status_t BClipboard::UploadToSystem()
{
}
//------------------------------------------------------------------------------

