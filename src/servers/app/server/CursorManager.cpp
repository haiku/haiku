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
#include "CursorManager.h"
#include "ServerCursor.h"
#include <Errors.h>

//! Initializes the CursorManager
CursorManager::CursorManager(void)
{
	_cursorlist=new BList(0);
	_lock=create_sem(1,"cursor_manager_sem");

	// Error code for AddCursor
	_tokenizer.ExcludeValue(B_ERROR);

	// Set system cursors to "unassigned"
	_defaultcsr=NULL;
	_textcsr=NULL;
	_movecsr=NULL;
	_dragcsr=NULL;
	_resizecsr=NULL;
	_resize_nwse_csr=NULL;
	_resize_nesw_csr=NULL;
	_resize_ns_csr=NULL;
	_resize_ew_csr=NULL;
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
	
	delete_sem(_lock);
	
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
	
	acquire_sem(_lock);
	_cursorlist->AddItem(sc);
	int32 value=_tokenizer.GetToken();
	sc->_token=value;
	release_sem(_lock);
	
	return value;
}

/*!
	\brief Removes a cursor from the internal list and deletes it
	\param token ID value of the cursor to be deleted
	
	If the cursor is not found, this call does nothing
*/
void CursorManager::DeleteCursor(int32 token)
{
	acquire_sem(_lock);

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
	release_sem(_lock);
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
	acquire_sem(_lock);

	ServerCursor *temp;
	for(int32 i=0; i<_cursorlist->CountItems();i++)
	{
		temp=(ServerCursor*)_cursorlist->ItemAt(i);
		if(temp && temp->_app_signature && 
			strcmp(signature, temp->_app_signature)==0)
		{
			_cursorlist->RemoveItem(i);
			delete temp;
			break;
		}
	}
	release_sem(_lock);
}

//! Wrapper around the DisplayDriver ShowCursor call
void CursorManager::ShowCursor(void)
{
	acquire_sem(_lock);
	// TODO: enable this when Desktop.h is added to server
//	DisplayDriver *driver=GetGfxDriver();
//	driver->ShowCursor();
	release_sem(_lock);
}

//! Wrapper around the DisplayDriver HideCursor call
void CursorManager::HideCursor(void)
{
	acquire_sem(_lock);
	// TODO: enable this when Desktop.h is added to server
//	DisplayDriver *driver=GetGfxDriver();
//	driver->HideCursor();
	release_sem(_lock);
}

//! Wrapper around the DisplayDriver ObscureCursor call
void CursorManager::ObscureCursor(void)
{
	acquire_sem(_lock);
	// TODO: enable this when Desktop.h is added to server
//	DisplayDriver *driver=GetGfxDriver();
//	driver->ObscureCursor();
	release_sem(_lock);
}

/*!
	\brief Set the screen's cursor
	\param token ID of the screen's new cursor
*/
void CursorManager::SetCursor(int32 token)
{
	acquire_sem(_lock);
	ServerCursor *c=_FindCursor(token);
	if(c)
	{
		// TODO: enable this when Desktop.h is added to server
//		DisplayDriver *driver=GetGfxDriver();
//		driver->SetCursor(c);
	}
	release_sem(_lock);
}

void CursorManager::SetCursor(cursor_which which)
{
	acquire_sem(_lock);

	// TODO: enable this when Desktop.h is added to server
/*	DisplayDriver *driver=GetGfxDriver();
	switch(which)
	{
		case B_CURSOR_DEFAULT:
		{
			driver->SetCursor(_defaultcsr);
			break;
		}
		case B_CURSOR_TEXT:
		{
			driver->SetCursor(_textcsr);
			break;
		}
		case B_CURSOR_MOVE:
		{
			driver->SetCursor(_movecsr);
			break;
		}
		case B_CURSOR_DRAG:
		{
			driver->SetCursor(_dragcsr);
			break;
		}
		case B_CURSOR_RESIZE:
		{
			driver->SetCursor(_resizecsr);
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			driver->SetCursor(_resize_nwse_csr);
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			driver->SetCursor(_resize_nesw_csr);
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			driver->SetCursor(_resize_ns_csr);
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			driver->SetCursor(_resize_ew_csr);
			break;
		}
		default:
			break;
	}
*/
	release_sem(_lock);
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
	
	acquire_sem(_lock);
	
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
	
	release_sem(_lock);
	return temp;
}

/*!
	\brief Gets the current system cursor value
	\return The current cursor value or CURSOR_OTHER if some non-system cursor
*/
cursor_which CursorManager::GetCursorWhich(void)
{
	cursor_which temp;
	
	acquire_sem(_lock);
	temp=_current_which;
	release_sem(_lock);
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
	acquire_sem(_lock);

	// Find the cursor, based on the token
	ServerCursor *cursor=_FindCursor(token);
	
	// Did we find a cursor with this token?
	if(!cursor)
	{
		release_sem(_lock);
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

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_TEXT:
		{
			if(_textcsr)
				delete _textcsr;

			_textcsr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_MOVE:
		{
			if(_movecsr)
				delete _movecsr;

			_movecsr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_DRAG:
		{
			if(_dragcsr)
				delete _dragcsr;

			_dragcsr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_RESIZE:
		{
			if(_resizecsr)
				delete _resizecsr;

			_resizecsr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_RESIZE_NWSE:
		{
			if(_resize_nwse_csr)
				delete _resize_nwse_csr;

			_resize_nwse_csr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_RESIZE_NESW:
		{
			if(_resize_nesw_csr)
				delete _resize_nesw_csr;

			_resize_nesw_csr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_RESIZE_NS:
		{
			if(_resize_ns_csr)
				delete _resize_ns_csr;

			_resize_ns_csr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		case B_CURSOR_RESIZE_EW:
		{
			if(_resize_ew_csr)
				delete _resize_ew_csr;

			_resize_ew_csr=cursor;

			if(cursor->_app_signature)
			{
				delete cursor->_app_signature;
				cursor->_app_signature=NULL;
			}
			break;
		}
		default:
			break;
	}
	
	release_sem(_lock);
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
