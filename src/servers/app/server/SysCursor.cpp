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
//	File Name:		SysCursor.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Private file encapsulating OBOS system cursor API
//  
//------------------------------------------------------------------------------
#include <PortLink.h>
#include <PortMessage.h>
#include <ServerProtocol.h>
#include <OS.h>
#include <String.h>
#include <File.h>
#include "SysCursor.h"
#include "ServerCursor.h"

/*!
	\brief Sets a system cursor
	\param which System cursor specifier defined in SysCursor.h
	\param cursor The new cursor
*/
void set_syscursor(cursor_which which, const BCursor *cursor)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_SYSCURSOR_BCURSOR);
		link.Attach<cursor_which>(which);
		
		// The easy (and clean) way for us to access the cursor's token
		// would be to make it a friend function of the BCursor class. One problem:
		// we couldn't build this under R5. For R1, we'll use a hack which we can
		// get away with because it has to be binary compatible.
		int32 *hack=(int32*)cursor;
		link.Attach<int32>(hack[0]);
//		link.Attach<int32>(cursor->m_serverToken);

		link.Flush();
	}
}

/*!
	\brief Sets a system cursor
	\param which System cursor specifier defined in SysCursor.h
	\param bitmap BBitmap to represent the new cursor. Size should be 48x48 or less.
*/
void set_syscursor(cursor_which which, const BBitmap *bitmap)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_SYSCURSOR_BBITMAP);
		link.Attach<cursor_which>(which);
		
		// Just like the BCursor version, we will use a hack until R1.
		int32 *hack=(int32*)bitmap;
		hack+=(sizeof(int32)*4)+sizeof(color_space)+sizeof(BRect);
		link.Attach<int32>(hack[0]);

		link.Flush();
	}
}

/*!
	\brief Returns the cursor specifier currently shown
	\return Returns B_CURSOR_OTHER if an application-set cursor. Otherwise, see SysCursor.h
*/
cursor_which get_syscursor(void)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortMessage pmsg;
		
		PortLink link(server);
		link.SetOpCode(AS_GET_SYSCURSOR);
		link.FlushWithReply(&pmsg);
		
		cursor_which which;
		pmsg.Read<cursor_which>(&which);
		return which;
	}
	return B_CURSOR_INVALID;
}

/*!
	\brief Changes the application cursor to the specified cursor
	\param which System cursor specifier defined in SysCursor.h
*/
void setcursor(cursor_which which)
{
	port_id server=find_port(SERVER_PORT_NAME);
	if(server!=B_NAME_NOT_FOUND)
	{
		PortLink link(server);
		link.SetOpCode(AS_SET_CURSOR_SYSTEM);
		link.Flush();
	}
}

/*!
	\brief Constructor
	\name Name of the cursor set.
*/
CursorSet::CursorSet(const char *name)
 : BMessage()
{
	AddString("name",(name)?name:"Untitled");
}

/*!
	\brief Saves the data in the cursor set to a file
	\param path Path of the file to save to.
	\param saveflags BFile open file flags. B_READ_WRITE is implied.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: path is NULL
	- \c other value: See BFile::SetTo and BMessage::Flatten return codes
*/
status_t CursorSet::Save(const char *path,int32 saveflags)
{
	if(!path)
		return B_BAD_VALUE;
	status_t stat=B_OK;

	BFile file;
	stat=file.SetTo(path,B_READ_WRITE | saveflags);
	if(stat!=B_OK)
		return stat;
	
	stat=Flatten(&file);
	
	return stat;
}

/*!
	\brief Loads the data into the cursor set from a file
	\param path Path of the file to load from.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: path is NULL
	- \c other value: See BFile::SetTo and BMessage::Flatten return codes
*/
status_t CursorSet::Load(const char *path)
{
	if(!path)
		return B_BAD_VALUE;
	status_t stat=B_OK;

	BFile file;
	stat=file.SetTo(path, B_READ_ONLY);
	if(stat!=B_OK)
		return stat;

	stat=Unflatten(&file);

	return stat;
}

/*!
	\brief Adds the cursor to the set and replaces any existing entry for the given specifier
	\param which System cursor specifier defined in SysCursor.h
	\param cursor BBitmap to represent the new cursor. Size should be 48x48 or less.
	\param hotspot The recipient of the hotspot for the cursor
	\return B_BAD_VALUE if cursor is NULL, otherwise B_OK
*/
status_t CursorSet::AddCursor(cursor_which which,const BBitmap *cursor, const BPoint &hotspot)
{
	if(!cursor)
		return B_BAD_VALUE;
	
	// Remove the data if it exists already
	RemoveData(CursorWhichToString(which));
	
	// Actually add the data to our set
	BMessage msg((int32)which);

	msg.AddString("class","bitmap");
	msg.AddRect("_frame",cursor->Bounds());
	msg.AddInt32("_cspace",cursor->ColorSpace());
	
	msg.AddInt32("_bmflags",0);
	msg.AddInt32("_rowbytes",cursor->BytesPerRow());
	msg.AddPoint("hotspot",hotspot);
	msg.AddData("_data",B_RAW_TYPE,cursor->Bits(), cursor->BitsLength());
	AddMessage(CursorWhichToString(which),&msg);
	
	return B_OK;
}

/*!
	\brief Adds the cursor to the set and replaces any existing entry for the given specifier
	\param which System cursor specifier defined in SysCursor.h
	\param data R5 cursor data pointer
	\return B_BAD_VALUE if data is NULL, otherwise B_OK
	
	When possible, it is better to use the BBitmap version of AddCursor because this 
	function must convert the R5 cursor data into a BBitmap
*/
status_t CursorSet::AddCursor(cursor_which which, int8 *data)
{
	// Convert cursor data to a bitmap because all cursors are internally stored
	// as bitmaps
	if(!data)
		return B_BAD_VALUE;
	
	BBitmap *bmp=CursorDataToBitmap(data);
	BPoint pt(data[2],data[3]);
	
	status_t stat=AddCursor(which,bmp,pt);
	
	if(bmp)
		delete bmp;
	
	return stat;
}

/*!
	\brief Removes the data associated with the specifier from the cursor set
	\param which System cursor specifier defined in SysCursor.h
*/
void CursorSet::RemoveCursor(cursor_which which)
{
	RemoveData(CursorWhichToString(which));
}

/*!
	\brief Retrieves a cursor from the set.
	\param which System cursor specifier defined in SysCursor.h
	\param cursor Bitmap** to receive a newly-allocated BBitmap containing the appropriate data
	\param hotspot The recipient of the hotspot for the cursor
	\return
	- \c B_OK: Success
	- \c B_BAD_VALUE: a NULL parameter was passed
	- \c B_NAME_NOT_FOUND: The specified cursor does not exist in this set
	- \c B_ERROR: An internal error occurred
	
	BBitmaps created by this function are the responsibility of the caller.
*/
status_t CursorSet::FindCursor(cursor_which which, BBitmap **cursor, BPoint *hotspot)
{

	if(!cursor || !hotspot);
		return B_BAD_VALUE;
	
	BMessage msg;

	if(FindMessage(CursorWhichToString(which),&msg)!=B_OK)
		return B_NAME_NOT_FOUND;
	
	const void *buffer;
	BString tempstr;
	int32 bufferLength;
	BBitmap *bmp;
	BPoint hotpt;
	
	if(msg.FindString("class",&tempstr)!=B_OK)
		return B_ERROR;
	
	if(msg.FindPoint("hotspot",&hotpt)!=B_OK)
		return B_ERROR;
	
		
	if(tempstr.Compare("cursor")==0)
	{
		bmp=new BBitmap( msg.FindRect("_frame"),
					(color_space) msg.FindInt32("_cspace"),true );
		msg.FindData("_data",B_RAW_TYPE,(const void **)&buffer, (ssize_t*)&bufferLength);
		memcpy(bmp->Bits(), buffer, bufferLength);

		*cursor=bmp;
		*hotspot=hotpt;
		return B_OK;
	}
	return B_ERROR;
}

/*!
	\brief Retrieves a cursor from the set.
	\param which System cursor specifier defined in SysCursor.h
	\param cursor ServerCursor** to receive a newly-allocated ServerCursor containing the appropriate data
	\return
	- \c B_OK: Success
	- \c B_BAD_VALUE: a NULL parameter was passed
	- \c B_NAME_NOT_FOUND: The specified cursor does not exist in this set
	- \c B_ERROR: An internal error occurred
	
	BBitmaps created by this function are the responsibility of the caller.
*/
status_t CursorSet::FindCursor(cursor_which which, ServerCursor **cursor)
{
	BMessage msg;
	
	if(FindMessage(CursorWhichToString(which),&msg)!=B_OK)
		return B_NAME_NOT_FOUND;
	
	const void *buffer;
	BString tempstr;
	int32 bufferLength;
	ServerCursor *csr;
	BPoint hotpt;
	
	if(msg.FindString("class",&tempstr)!=B_OK)
		return B_ERROR;
	
	if(msg.FindPoint("hotspot",&hotpt)!=B_OK)
		return B_ERROR;

	if(tempstr.Compare("cursor")==0)
	{
		csr=new ServerCursor(msg.FindRect("_frame"),(color_space) msg.FindInt32("_cspace"),0, hotpt);
		msg.FindData("_data",B_RAW_TYPE,(const void **)&buffer, (ssize_t*)&bufferLength);
		memcpy(csr->Bits(), buffer, bufferLength);

		*cursor=csr;
		return B_OK;
	}
	return B_ERROR;
}

/*!
	\brief Returns the name of the set
	\return The name of the set
*/
const char *CursorSet::GetName(void)
{
	BString name;
	if(FindString("name",&name)==B_OK)
		return name.String();
	return NULL;
}

/*!
	\brief Renames the cursor set
	\param name new name of the set.
	
	This function will fail if given a NULL name
*/
void CursorSet::SetName(const char *name)
{
	if(!name)
		return;
	RemoveData("name");
	AddString("name",name);
}


/*!
	\brief Returns a string for the specified cursor attribute
	\param which System cursor specifier defined in SysCursor.h
	\return Name for the cursor specifier
*/
const char *CursorWhichToString(cursor_which which)
{
	switch(which)
	{
		case B_CURSOR_DEFAULT:
			return "Default";
		case B_CURSOR_TEXT:
			return "Text";
		case B_CURSOR_MOVE:
			return "Move";
		case B_CURSOR_DRAG:
			return "Drag";
		case B_CURSOR_RESIZE:
			return "Resize";
		case B_CURSOR_RESIZE_NWSE:
			return "Resize NWSE";
		case B_CURSOR_RESIZE_NESW:
			return "Resize NESW";
		case B_CURSOR_RESIZE_NS:
			return "Resize NS";
		case B_CURSOR_RESIZE_EW:
			return "Resize EW";
		case B_CURSOR_OTHER:
			return "Other";
		default:
			return "Invalid";
	}
}

/*!
	\brief Creates a new BBitmap from R5 cursor data
	\param data Pointer to data in the R5 cursor data format
	\return NULL if data was NULL, otherwise a new BBitmap
	
	BBitmaps returned by this function are always in the RGBA32 color space
*/
BBitmap *CursorDataToBitmap(int8 *data)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	if(data)
	{	
	 	BBitmap *bmp=new BBitmap(BRect(0,0,15,15),B_RGBA32,0);
		uint32 black=0xFF000000,
			white=0xFFFFFFFF,
			*bmppos;
		uint16	*cursorpos, *maskpos,cursorflip, maskflip,
				cursorval, maskval,powval;
		uint8 	i,j,*_buffer;
		_buffer=(uint8*)bmp->Bits();
		cursorpos=(uint16*)(data+4);
		maskpos=(uint16*)(data+36);
		
		// for each row in the cursor data
		for(j=0;j<16;j++)
		{
			bmppos=(uint32*)(_buffer+ (j*bmp->BytesPerRow()) );
	
			// On intel, our bytes end up swapped, so we must swap them back
			cursorflip=(cursorpos[j] & 0xFF) << 8;
			cursorflip |= (cursorpos[j] & 0xFF00) >> 8;
			
			maskflip=(maskpos[j] & 0xFF) << 8;
			maskflip |= (maskpos[j] & 0xFF00) >> 8;
	
			// for each column in each row of cursor data
			for(i=0;i<16;i++)
			{
				// Get the values and dump them to the bitmap
				powval=1 << (15-i);
				cursorval=cursorflip & powval;
				maskval=maskflip & powval;
				bmppos[i]=((cursorval!=0)?black:white) & ((maskval>0)?0xFFFFFFFF:0x00FFFFFF);
			}
		}
		return bmp;
	}

	return NULL;
}
