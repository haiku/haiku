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
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Clipboard.h>
#include <Application.h>
#include <RegistrarDefs.h>
#include <RosterPrivate.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


//------------------------------------------------------------------------------
BClipboard::BClipboard(const char *name, bool transient)
{
  if ( name )
    fName = strdup(name);
  else
    fName = strdup("system");
  fData = new BMessage();
  fCount = 0;
  fSystemCount = 0;

  BMessage message(B_REG_GET_CLIPBOARD_MESSENGER), reply;
  if ( (BRoster::Private().SendTo(&message, &reply, false) == B_OK) &&
       (reply.what == B_REG_SUCCESS) &&
       (reply.FindMessenger("messenger",&fClipHandler) == B_OK) )
  {
    BMessage handlerMessage(B_REG_ADD_CLIPBOARD), handlerReply;
    int32 result;
    if ( (handlerMessage.AddString("name",fName) == B_OK) &&
         (fClipHandler.SendMessage(&handlerMessage, &handlerReply) == B_OK) )
      handlerReply.FindInt32("result",&result);
  }
}
//------------------------------------------------------------------------------
BClipboard::~BClipboard()
{
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
  /* fSystemCount contains the total number of writes to the clipboard.
     fCount contains the number of writes to the clipboard done by this
     BClipboard.
  */
  return fSystemCount;
}
//------------------------------------------------------------------------------
uint32 BClipboard::SystemCount() const
{
  int32 val;
  BMessage message(B_REG_GET_CLIPBOARD_COUNT), reply;
  if ( (message.AddString("name",fName) == B_OK) &&
       (fClipHandler.SendMessage(&message, &reply) == B_OK) &&
       (reply.FindInt32("count",&val) == B_OK) )
    return (uint32)val;

  return 0;
}
//------------------------------------------------------------------------------
status_t BClipboard::StartWatching(BMessenger target)
{
  BMessage message(B_REG_CLIPBOARD_START_WATCHING), reply;
  if ( (message.AddString("name",fName) == B_OK) &&
       (message.AddMessenger("target", target ) == B_OK) &&
       (fClipHandler.SendMessage(&message, &reply) == B_OK) )
  {
    int32 result;
    reply.FindInt32("result",&result);
    return result;
  }
  return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BClipboard::StopWatching(BMessenger target)
{
  BMessage message(B_REG_CLIPBOARD_STOP_WATCHING), reply;
  if ( (message.AddString("name",fName) == B_OK) &&
       (message.AddMessenger("target", target ) == B_OK) &&
       (fClipHandler.SendMessage(&message, &reply) == B_OK) )
  {
    int32 result;
    reply.FindInt32("result",&result);
    return result;
  }
  return B_ERROR;
}
//------------------------------------------------------------------------------
bool BClipboard::Lock()
{
  /* Will this work correctly if clipboard is deleted while still waiting on
     fLock.Lock() ? */
  bool retVal;
  retVal = fLock.Lock();
  if ( retVal  &&
      (DownloadFromSystem() != B_OK) )
  {
    retVal = false;
    fLock.Unlock();
  }

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
  if ( AssertLocked() &&
       (fData->MakeEmpty() == B_OK) )
    return B_OK;
  return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BClipboard::Commit()
{
  if ( AssertLocked() &&
       (UploadToSystem() == B_OK) )
    return B_OK;
  return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BClipboard::Revert()
{
  if ( AssertLocked() &&
       (fData->MakeEmpty() == B_OK) &&
       (DownloadFromSystem() == B_OK) )
    return B_OK;
  return B_ERROR;
}
//------------------------------------------------------------------------------
BMessenger BClipboard::DataSource() const
{
  return fDataSource;
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
  /* This is private, and I don't use it, so I'm not going to implement it */
}
//------------------------------------------------------------------------------
BClipboard & BClipboard::operator=(const BClipboard &)
{
  /* This is private, and I don't use it, so I'm not going to implement it */
	return *this;
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
	// This function is for jumping to the debugger if not locked
	if(!fLock.IsLocked())
	{
		debugger("The clipboard must be locked before proceeding.");
		return false;
	}
	return true;
}
//------------------------------------------------------------------------------
status_t BClipboard::DownloadFromSystem(bool force)
{
	// Apparently, the force paramater was used in some sort of
	// optimization in R5. Currently, we ignore it.
  BMessage message(B_REG_DOWNLOAD_CLIPBOARD), reply;
  if ( (message.AddString("name",fName) == B_OK) &&
       (fClipHandler.SendMessage(&message, &reply) == B_OK) &&
       (reply.FindMessage("data",fData) == B_OK) &&
       (reply.FindMessenger("data source",&fDataSource) == B_OK) &&
       (reply.FindInt32("count",(int32 *)(&fSystemCount)) == B_OK) )
    return B_OK;
  return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BClipboard::UploadToSystem()
{
  BMessage message(B_REG_UPLOAD_CLIPBOARD), reply;
  if ( (message.AddString("name",fName) == B_OK) &&
       (message.AddMessage("data",fData) == B_OK) &&
       (message.AddMessenger("data source", be_app_messenger ) == B_OK) &&
       (fClipHandler.SendMessage(&message, &reply) == B_OK) &&
       (reply.FindInt32("count",(int32 *)(&fSystemCount)) == B_OK) )
  {
    fCount++;
    return B_OK;
  }
  return B_ERROR;
}
//------------------------------------------------------------------------------


