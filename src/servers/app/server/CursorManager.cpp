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
	_cursorlist=new BList(0);

	// Error code for AddCursor
	_tokenizer.ExcludeValue(B_ERROR);

	// Set system cursors to "unassigned"
	ServerCursor *cdefault=new ServerCursor(default_cursor_data);
	AddCursor(cdefault);
	_defaultcsr=cdefault;
	
	ServerCursor *ctext=new ServerCursor(default_text_data);
	AddCursor(ctext);
	_textcsr=ctext;

	ServerCursor *cmove=new ServerCursor(default_move_data);
	AddCursor(cmove);
	_movecsr=cmove;

	ServerCursor *cdrag=new ServerCursor(default_drag_data);
	AddCursor(cdrag);
	_dragcsr=cdrag;

	ServerCursor *cresize=new ServerCursor(default_resize_data);
	AddCursor(cresize);
	_resizecsr=cresize;

	ServerCursor *cresizenwse=new ServerCursor(default_resize_nwse_data);
	AddCursor(cresizenwse);
	_resize_nwse_csr=cresizenwse;

	ServerCursor *cresizenesw=new ServerCursor(default_resize_nesw_data);
	AddCursor(cresizenesw);
	_resize_nesw_csr=cresizenesw;

	ServerCursor *cresizens=new ServerCursor(default_resize_ns_data);
	AddCursor(cresizens);
	_resize_ns_csr=cresizens;

	ServerCursor *cresizeew=new ServerCursor(default_resize_ew_data);
	AddCursor(cresizeew);
	_resize_ew_csr=cresizeew;

}

//! Does all the teardown
CursorManager::~CursorManager(void)
{
	ServerCursor *temp;
	for(int32 i=0; i<_cursorlist->CountItems();i++)
	{
		temp=(ServerCursor*)_cursorlist->ItemAt(i);
		if(temp)
			delete temp;
	}
	_cursorlist->MakeEmpty();
	delete _cursorlist;
	
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
	_cursorlist->AddItem(sc);
	int32 value=_tokenizer.GetToken();
	sc->_token=value;
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
	for(int32 i=0; i<_cursorlist->CountItems();i++)
	{
		temp=(ServerCursor*)_cursorlist->ItemAt(i);
		if(temp && temp->_token==token)
		{
			_cursorlist->RemoveItem(i);
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
void CursorManager::RemoveAppCursors(const char *signature)
{
	// OPTIMIZATION: For an optimization, it perhaps may be wise down 
	// the road to replace the ServerCursor's app signature with a 
	// pointer to its application and compare ServerApp pointers instead.
	Lock();

	ServerCursor *temp;
	for(int32 i=0; i<_cursorlist->CountItems();i++)
	{
		temp=(ServerCursor*)_cursorlist->ItemAt(i);
		if(temp && temp->GetAppSignature() && 
			strcmp(signature, temp->GetAppSignature())==0)
		{
			_cursorlist->RemoveItem(i);
			delete temp;
			break;
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
	ServerCursor *c=_FindCursor(token);
	if(c)
	{
		DisplayDriver *driver = desktop->ActiveScreen()->DDriver();
		driver->SetCursor(c);
		_current_which=B_CURSOR_OTHER;
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
			driver->SetCursor(_defaultcsr);
			_current_which=which;
			break;
		}
		case B_CURSOR_TEXT:
		{
			driver->SetCursor(_textcsr);
			_current_which=which;
			break;
		}
		case B_CURSOR_MOVE:
		{
			driver->SetCursor(_movecsr);
			_current_which=which;
			break;
		}
		case B_CURSOR_DRAG:
		{
			driver->SetCursor(_dragcsr);
			_current_which=which;
			break;
		}
		case B_CURSOR_RESIZE:
		{
			driver->SetCursor(_resizecsr);
			_current_which=which;
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			driver->SetCursor(_resize_nwse_csr);
			_current_which=which;
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			driver->SetCursor(_resize_nesw_csr);
			_current_which=which;
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			driver->SetCursor(_resize_ns_csr);
			_current_which=which;
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			driver->SetCursor(_resize_ew_csr);
			_current_which=which;
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
		if(_defaultcsr)
			delete _defaultcsr;
		_defaultcsr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_TEXT,&csr)==B_OK)
	{
		if(_textcsr)
			delete _textcsr;
		_textcsr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_MOVE,&csr)==B_OK)
	{
		if(_movecsr)
			delete _movecsr;
		_movecsr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_DRAG,&csr)==B_OK)
	{
		if(_dragcsr)
			delete _dragcsr;
		_dragcsr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE,&csr)==B_OK)
	{
		if(_resizecsr)
			delete _resizecsr;
		_resizecsr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NWSE,&csr)==B_OK)
	{
		if(_resize_nwse_csr)
			delete _resize_nwse_csr;
		_resize_nwse_csr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NESW,&csr)==B_OK)
	{
		if(_resize_nesw_csr)
			delete _resize_nesw_csr;
		_resize_nesw_csr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_NS,&csr)==B_OK)
	{
		if(_resize_ns_csr)
			delete _resize_ns_csr;
		_resize_ns_csr=csr;
	}
	
	if(cs.FindCursor(B_CURSOR_RESIZE_EW,&csr)==B_OK)
	{
		if(_resize_ew_csr)
			delete _resize_ew_csr;
		_resize_ew_csr=csr;
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
			temp=_defaultcsr;
			break;
		}
		case B_CURSOR_TEXT:
		{
			temp=_textcsr;
			break;
		}
		case B_CURSOR_MOVE:
		{
			temp=_movecsr;
			break;
		}
		case B_CURSOR_DRAG:
		{
			temp=_dragcsr;
			break;
		}
		case B_CURSOR_RESIZE:
		{
			temp=_resizecsr;
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			temp=_resize_nwse_csr;
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			temp=_resize_nesw_csr;
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			temp=_resize_ns_csr;
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			temp=_resize_ew_csr;
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
	temp=_current_which;
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
	ServerCursor *cursor=_FindCursor(token);
	
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
			if(_defaultcsr)
				delete _defaultcsr;

			_defaultcsr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");

			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_TEXT:
		{
			if(_textcsr)
				delete _textcsr;

			_textcsr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_MOVE:
		{
			if(_movecsr)
				delete _movecsr;

			_movecsr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_DRAG:
		{
			if(_dragcsr)
				delete _dragcsr;

			_dragcsr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE:
		{
			if(_resizecsr)
				delete _resizecsr;

			_resizecsr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			if(_resize_nwse_csr)
				delete _resize_nwse_csr;

			_resize_nwse_csr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			if(_resize_nesw_csr)
				delete _resize_nesw_csr;

			_resize_nesw_csr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			if(_resize_ns_csr)
				delete _resize_ns_csr;

			_resize_ns_csr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			if(_resize_ew_csr)
				delete _resize_ew_csr;

			_resize_ew_csr=cursor;

			if(cursor->GetAppSignature())
				cursor->SetAppSignature("");
			_cursorlist->RemoveItem(cursor);
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
ServerCursor *CursorManager::_FindCursor(int32 token)
{
	ServerCursor *temp;
	for(int32 i=0; i<_cursorlist->CountItems();i++)
	{
		temp=(ServerCursor*)_cursorlist->ItemAt(i);
		if(temp && temp->_token==token)
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

