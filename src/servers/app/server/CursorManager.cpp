//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		CursorManager.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Handles the system's cursor infrastructure
//  
//------------------------------------------------------------------------------
#include "DisplayDriver.h"
#include "Desktop.h"
#include "CursorManager.h"
#include "ServerCursor.h"
#include "CursorData.h"
#include "ServerScreen.h"
#include "ServerConfig.h"
#include <Errors.h>
#include <Directory.h>
#include <String.h>
#include <string.h>

//! The global cursor manager object. Allocated and freed by AppServer class
CursorManager *cursormanager;

//! Initializes the CursorManager
CursorManager::CursorManager(void)
{
	fCursorList=new BList(0);

	// Error code for AddCursor
	fTokenizer.ExcludeValue(B_ERROR);

	// Set system cursors to "unassigned"
	ServerCursor *cdefault=new ServerCursor(default_cursor_data);
	AddCursor(cdefault);
	fDefaultCursor=cdefault;
	
	ServerCursor *ctext=new ServerCursor(default_text_data);
	AddCursor(ctext);
	fTextCursor=ctext;

	ServerCursor *cmove=new ServerCursor(default_move_data);
	AddCursor(cmove);
	fMoveCursor=cmove;

	ServerCursor *cdrag=new ServerCursor(default_drag_data);
	AddCursor(cdrag);
	fDragCursor=cdrag;

	ServerCursor *cresize=new ServerCursor(default_resize_data);
	AddCursor(cresize);
	fResizeCursor=cresize;

	ServerCursor *cresizenwse=new ServerCursor(default_resize_nwse_data);
	AddCursor(cresizenwse);
	fNWSECursor=cresizenwse;

	ServerCursor *cresizenesw=new ServerCursor(default_resize_nesw_data);
	AddCursor(cresizenesw);
	fNESWCursor=cresizenesw;

	ServerCursor *cresizens=new ServerCursor(default_resize_ns_data);
	AddCursor(cresizens);
	fNSCursor=cresizens;

	ServerCursor *cresizeew=new ServerCursor(default_resize_ew_data);
	AddCursor(cresizeew);
	fEWCursor=cresizeew;

}

//! Does all the teardown
CursorManager::~CursorManager(void)
{
	ServerCursor *temp;
	for(int32 i=0; i<fCursorList->CountItems();i++)
	{
		temp=(ServerCursor*)fCursorList->ItemAt(i);
		if(temp)
			delete temp;
	}
	fCursorList->MakeEmpty();
	delete fCursorList;
	
	// Note that it is not necessary to remove and delete the system
	// cursors. These cursors are kept in the list with a NULL application
	// signature so they cannot be removed very easily except via
	// SetCursor(cursor_which). At shutdown, they are removed with the 
	// above loop.
}

/*!
	\brief Registers a cursor with the manager.
	\param sc ServerCursor object to register
	\return The token assigned to the cursor or B_ERROR if sc is NULL
*/
int32 CursorManager::AddCursor(ServerCursor *sc)
{
	if(!sc)
		return B_ERROR;
	
	Lock();
	fCursorList->AddItem(sc);
	int32 value=fTokenizer.GetToken();
	sc->fToken=value;
	Unlock();
	
	return value;
}

/*!
	\brief Removes a cursor from the internal list and deletes it
	\param token ID value of the cursor to be deleted
	
	If the cursor is not found, this call does nothing
*/
void CursorManager::DeleteCursor(int32 token)
{
	Lock();

	ServerCursor *temp;
	for(int32 i=0; i<fCursorList->CountItems();i++)
	{
		temp=(ServerCursor*)fCursorList->ItemAt(i);
		if(temp && temp->fToken==token)
		{
			fCursorList->RemoveItem(i);
			delete temp;
			break;
		}
	}
	Unlock();
}

/*!
	\brief Removes and deletes all of an application's cursors
	\param signature Signature to which the cursors belong
*/
void CursorManager::RemoveAppCursors(team_id team)
{
	Lock();

	ServerCursor *temp;
	for(int32 i=0; i<fCursorList->CountItems();i++)
	{
		temp=(ServerCursor*)fCursorList->ItemAt(i);
		if(temp && temp->OwningTeam()==team)
		{
			fCursorList->RemoveItem(i);
			delete temp;
		}
	}
	Unlock();
}

//! Wrapper around the DisplayDriver ShowCursor call
void CursorManager::ShowCursor(void)
{
	Lock();

	DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
	driver->ShowCursor();
	Unlock();
}

//! Wrapper around the DisplayDriver HideCursor call
void CursorManager::HideCursor(void)
{
	Lock();

	DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
	driver->HideCursor();
	Unlock();
}

//! Wrapper around the DisplayDriver ObscureCursor call
void CursorManager::ObscureCursor(void)
{
	Lock();

	DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
	driver->ObscureCursor();
	Unlock();
}

/*!
	\brief Set the screen's cursor
	\param token ID of the screen's new cursor
*/
void CursorManager::SetCursor(int32 token)
{
	Lock();
	ServerCursor *c=FindCursor(token);
	if(c)
	{
		DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
		driver->SetCursor(c);
		fCurrentWhich=B_CURSOR_OTHER;
	}
	Unlock();
}

void CursorManager::SetCursor(cursor_which which)
{
	Lock();

	DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
	switch(which)
	{
		case B_CURSOR_DEFAULT:
		{
			driver->SetCursor(fDefaultCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_TEXT:
		{
			driver->SetCursor(fTextCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_MOVE:
		{
			driver->SetCursor(fMoveCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_DRAG:
		{
			driver->SetCursor(fDragCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_RESIZE:
		{
			driver->SetCursor(fResizeCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			driver->SetCursor(fNWSECursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			driver->SetCursor(fNESWCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			driver->SetCursor(fNSCursor);
			fCurrentWhich=which;
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			driver->SetCursor(fEWCursor);
			fCurrentWhich=which;
			break;
		}
		default:
			break;
	}

	Unlock();
}

/*!
	\brief Sets all the cursors from a specified CursorSet
	\param path Path to the cursor set
	
	All cursors in the set will be assigned. If the set does not specify a cursor for a
	particular cursor specifier, it will remain unchanged. This function will fail if passed
	a NULL path, an invalid path, or the path to a non-CursorSet file.
*/
void CursorManager::SetCursorSet(const char *path)
{
	Lock();
	CursorSet cs(NULL);
	
	if(!path || cs.Load(path)!=B_OK)
		return;
	ServerCursor *csr;
	
	if(cs.FindCursor(B_CURSOR_DEFAULT,&csr)==B_OK)
	{
		if(fDefaultCursor)
			delete fDefaultCursor;
		fDefaultCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_TEXT,&csr)==B_OK)
	{
		if(fTextCursor)
			delete fTextCursor;
		fTextCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_MOVE,&csr)==B_OK)
	{
		if(fMoveCursor)
			delete fMoveCursor;
		fMoveCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_DRAG,&csr)==B_OK)
	{
		if(fDragCursor)
			delete fDragCursor;
		fDragCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE,&csr)==B_OK)
	{
		if(fResizeCursor)
			delete fResizeCursor;
		fResizeCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NWSE,&csr)==B_OK)
	{
		if(fNWSECursor)
			delete fNWSECursor;
		fNWSECursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NESW,&csr)==B_OK)
	{
		if(fNESWCursor)
			delete fNESWCursor;
		fNESWCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NS,&csr)==B_OK)
	{
		if(fNSCursor)
			delete fNSCursor;
		fNSCursor=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_EW,&csr)==B_OK)
	{
		if(fEWCursor)
			delete fEWCursor;
		fEWCursor=csr;
	}
	Unlock();
	
}

/*!
	\brief Acquire the cursor which is used for a particular system cursor
	\param which Which system cursor to get
	\return Pointer to the particular cursor used or NULL if which is 
	invalid or the cursor has not been assigned
*/
ServerCursor *CursorManager::GetCursor(cursor_which which)
{
	ServerCursor *temp=NULL;
	
	Lock();
	
	switch(which)
	{
		case B_CURSOR_DEFAULT:
		{
			temp=fDefaultCursor;
			break;
		}
		case B_CURSOR_TEXT:
		{
			temp=fTextCursor;
			break;
		}
		case B_CURSOR_MOVE:
		{
			temp=fMoveCursor;
			break;
		}
		case B_CURSOR_DRAG:
		{
			temp=fDragCursor;
			break;
		}
		case B_CURSOR_RESIZE:
		{
			temp=fResizeCursor;
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			temp=fNWSECursor;
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			temp=fNESWCursor;
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			temp=fNSCursor;
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			temp=fEWCursor;
			break;
		}
		default:
			break;
	}
	
	Unlock();
	return temp;
}

/*!
	\brief Gets the current system cursor value
	\return The current cursor value or CURSOR_OTHER if some non-system cursor
*/
cursor_which CursorManager::GetCursorWhich(void)
{
	cursor_which temp;
	
	Lock();
	temp=fCurrentWhich;
	Unlock();
	return temp;
}

/*!
	\brief Sets the specified system cursor to the a particular cursor
	\param which Which system cursor to change
	\param token The ID of the cursor to become the new one
	
	A word of warning: once a cursor has been assigned to the system, the
	system will take ownership of the cursor and deleting the cursor
	will have not effect on the system.
*/
void CursorManager::ChangeCursor(cursor_which which, int32 token)
{
	Lock();

	// Find the cursor, based on the token
	ServerCursor *cursor=FindCursor(token);
	
	// Did we find a cursor with this token?
	if(!cursor)
	{
		Unlock();
		return;
	}

	// Do the assignment
	switch(which)
	{
		case B_CURSOR_DEFAULT:
		{
			if(fDefaultCursor)
				delete fDefaultCursor;

			fDefaultCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");

			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_TEXT:
		{
			if(fTextCursor)
				delete fTextCursor;

			fTextCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_MOVE:
		{
			if(fMoveCursor)
				delete fMoveCursor;

			fMoveCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_DRAG:
		{
			if(fDragCursor)
				delete fDragCursor;

			fDragCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE:
		{
			if(fResizeCursor)
				delete fResizeCursor;

			fResizeCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			if(fNWSECursor)
				delete fNWSECursor;

			fNWSECursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			if(fNESWCursor)
				delete fNESWCursor;

			fNESWCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			if(fNSCursor)
				delete fNSCursor;

			fNSCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			if(fEWCursor)
				delete fEWCursor;

			fEWCursor=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			fCursorList->RemoveItem(cursor);
			break;
		}
		default:
			break;
	}
	
	Unlock();
}

/*!
	\brief Internal function which finds the cursor with a particular ID
	\param token ID of the cursor to find
	\return The cursor or NULL if not found
*/
ServerCursor *CursorManager::FindCursor(int32 token)
{
	ServerCursor *temp;
	for(int32 i=0; i<fCursorList->CountItems();i++)
	{
		temp=(ServerCursor*)fCursorList->ItemAt(i);
		if(temp && temp->fToken==token)
			return temp;
	}
	return NULL;
}

//! Sets the cursors to the defaults and saves them to CURSOR_SETTINGS_DIR/"d
void CursorManager::SetDefaults(void)
{
	Lock();
	CursorSet cs("Default");
	cs.AddCursor(B_CURSOR_DEFAULT,default_cursor_data);
	cs.AddCursor(B_CURSOR_TEXT,default_text_data);
	cs.AddCursor(B_CURSOR_MOVE,default_move_data);
	cs.AddCursor(B_CURSOR_DRAG,default_drag_data);
	cs.AddCursor(B_CURSOR_RESIZE,default_resize_data);
	cs.AddCursor(B_CURSOR_RESIZE_NWSE,default_resize_nwse_data);
	cs.AddCursor(B_CURSOR_RESIZE_NESW,default_resize_nesw_data);
	cs.AddCursor(B_CURSOR_RESIZE_NS,default_resize_ns_data);
	cs.AddCursor(B_CURSOR_RESIZE_EW,default_resize_ew_data);

	BDirectory dir;
	if(dir.SetTo(CURSOR_SET_DIR)==B_ENTRY_NOT_FOUND)
		create_directory(CURSOR_SET_DIR,0777);
	
	BString string(CURSOR_SET_DIR);
	string+="Default";
	cs.Save(string.String(),B_CREATE_FILE | B_FAIL_IF_EXISTS);
	
	SetCursorSet(string.String());
	Unlock();
}

