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
//	File Name:		PortLink.cpp
//	Author:			Pahtz <pahtz@yahoo.com.au>
//	Description:	Class for low-overhead port-based messaging
//  
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>
#include "PortLink.h"

#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
	const char *strcode(int32 code);
#else
#	define STRACE(x) ;
#endif

//set Initial==Max for a fixed buffer size
static const int32 kInitialSendBufferSize = 2048;
static const int32 kMaxSendBufferSize = 2048;
static const int32 kInitialReceiveBufferSize = 2048;
static const int32 kMaxReceiveBufferSize = 2048;
//make the max receive buffer at least as large as max send

static const int32 kHeaderSize = sizeof(int32) * 3; //size + code + flags

BPortLink::BPortLink(port_id send, port_id receive) :
	fSendPort(send), fReceivePort(receive), fSendBuffer(NULL), fRecvBuffer(NULL),
	fSendPosition(0), fRecvPosition(0), fSendStart(0), fRecvStart(0),
	fSendBufferSize(0), fRecvBufferSize(0), fSendCount(0), fDataSize(0),
	fReplySize(0), fWriteError(B_OK), fReadError(B_OK)
{
	/*	*/
}

BPortLink::~BPortLink()
{
	if (fSendBuffer)
		free(fSendBuffer);
	if (fRecvBuffer)
		free(fRecvBuffer);
}

status_t BPortLink::StartMessage(int32 code)
{
	if (EndMessage() < B_OK)	//end previous message
		CancelMessage();	//abandon previous message

	if (fSendBufferSize == 0)
	{
		fSendBuffer = (char *)malloc(kInitialSendBufferSize);
		if (fSendBuffer == NULL)
		{
			fWriteError = B_NO_MEMORY;
			return B_NO_MEMORY;
		}
		fSendBufferSize = kInitialSendBufferSize;
	}

	status_t err;
	//must have space for at least size + code + flags
	if (fSendBufferSize - fSendPosition < kHeaderSize)
	{
		err = Flush();	//will set fSendPosition and fSendStart to 0
		if (err < B_OK)
			return err;
	}

	int32 *p = (int32 *)(fSendBuffer + fSendStart);	//start of current message
	*p = 0;	//size
	*(++p) = code;	//code
	*(++p) = 0;	//flags
	fSendPosition += kHeaderSize;	//size + code + flags
	
	STRACE(("info: BPortLink buffered header %s [%ld %ld %ld].\n", strcode(code), (int32)0, *(p-1), *p));

	return B_OK;
}

status_t BPortLink::EndMessage()
{
	if (fSendPosition == fSendStart || fWriteError < B_OK)
		return fWriteError;

	int32 *p = (int32 *)(fSendBuffer + fSendStart);	//start of the message
	*p = fSendPosition - fSendStart;	//record the size of the message
	fSendCount++;	//increase the number of completed messages

	fSendStart = fSendPosition;	//start of next new message
	
	return B_OK;
	STRACE(("info: BPortLink EndMessage() of size %ld.\n", *p));
}

void BPortLink::CancelMessage()
{
	fSendPosition = fSendStart;
	fWriteError = B_OK;
}

status_t BPortLink::Attach(const void *data, ssize_t size)
{
	if (fWriteError < B_OK)
		return fWriteError;

	if (size <= 0)
	{
		fWriteError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	if (fSendPosition == fSendStart)
		return B_NO_INIT;	//need to call StartMessage() first
  
	int32 remaining = fSendBufferSize - fSendPosition;
	if (remaining < size)	//we have to make space for the data
	{
		int32 total = size + (fSendPosition - fSendStart);
			//resulting size of current message

		int32 newbuffersize;
		if (total <= fSendBufferSize)
			newbuffersize = fSendBufferSize;	//no change
		else if (total > kMaxSendBufferSize)
		{
			fWriteError = B_BAD_VALUE;
			return B_BAD_VALUE;
		}
		else if (total <= kInitialSendBufferSize)
			newbuffersize = kInitialSendBufferSize;
		else
			newbuffersize = (total + B_PAGE_SIZE) - (total % B_PAGE_SIZE);

		//FlushCompleted() to make space
		status_t err;
		err = FlushCompleted(newbuffersize);
		if (err < B_OK)
		{
			fWriteError = err;
			return err;
		}
	}

	memcpy(fSendBuffer + fSendPosition, data, size);
	fSendPosition += size;
	return fWriteError;
}

status_t BPortLink::FlushCompleted(ssize_t newbuffersize)
{
	char *buffer = NULL;
	if (newbuffersize == fSendBufferSize)
		buffer = fSendBuffer;	//keep existing buffer
	else
	{
		//create new larger buffer
		buffer = (char *)malloc(newbuffersize);
		if (buffer == NULL)
			return B_NO_MEMORY;
	}

	int32 position = fSendPosition;
	int32 start = fSendStart;
	fSendPosition = fSendStart;	//trick to hide the incomplete message

	status_t err;
	err = Flush();
	if (err < B_OK)
	{
		fSendPosition = position;
		if (buffer != fSendBuffer)
			free(buffer);
		return err;
	}

	//move the incomplete message to the start of the buffer
	fSendPosition = min_c(position - start, newbuffersize);
	memcpy(buffer, fSendBuffer + start, fSendPosition);

	if (fSendBuffer != buffer)
	{
		free(fSendBuffer);
		fSendBuffer = buffer;
		fSendBufferSize = newbuffersize;
	}

	return B_OK;
}


void BPortLink::SetSendPort( port_id port )
{
	fSendPort=port;
}

port_id BPortLink::GetSendPort()
{
	return fSendPort;
}

void BPortLink::SetReplyPort( port_id port )
{
	fReceivePort=port;
}

port_id BPortLink::GetReplyPort()
{
	return fReceivePort;
}

status_t BPortLink::Flush(bigtime_t timeout)
{
	if (fWriteError < B_OK)
		return fWriteError;

	EndMessage();
	if (fSendCount == 0)
		return B_OK;

	STRACE(("info: BPortLink Flush() waiting to send %ld messages of %ld bytes on port %ld.\n", fSendCount, fSendPosition, fSendPort));
	
	//TODO: we only need AS_SERVER_PORTLINK when all OBOS uses BPortLink
	int32 protocol = (fSendCount > 1 ? AS_SERVER_SESSION : AS_SERVER_PORTLINK);

	status_t err;
	if(timeout != B_INFINITE_TIMEOUT)
	{
		do {
			err = write_port_etc(fSendPort, protocol, fSendBuffer,
									fSendPosition, B_RELATIVE_TIMEOUT, timeout);
		} while(err == B_INTERRUPTED);
	}
	else
	{
		do {
			err = write_port(fSendPort, protocol, fSendBuffer, fSendPosition);
		} while(err == B_INTERRUPTED);
	}
	
	if (err == B_OK)
	{
		STRACE(("info: BPortLink Flush() %ld messages total of %ld bytes on port %ld.\n", fSendCount, fSendPosition, fSendPort));
		fSendPosition = 0;
		fSendStart = 0;
		fSendCount = 0;
		return B_OK;
	}

	STRACE(("error info: BPortLink Flush() failed for %ld bytes (%s) on port %ld.\n", fSendPosition, strerror(err), fSendPort));
	return err;
}

status_t BPortLink::GetNextReply(int32 *code, bigtime_t timeout)
{
	int32 remaining;

	fReadError = B_OK;

	remaining = fDataSize - (fRecvStart + fReplySize);
	STRACE(("info: BPortLink GetNextReply() reports %ld bytes remaining in buffer.\n", remaining));

	//find the position of the next message header in the buffer
	int32 *header;
	if (remaining <= 0)
	{
		status_t err = ReadFromPort(timeout);
		if (err < B_OK)
			return err;
		remaining = fDataSize;
		header = (int32 *)fRecvBuffer;
	}
	else
	{
		fRecvStart += fReplySize;	//start of the next message
		fRecvPosition = fRecvStart;
		header = (int32 *)(fRecvBuffer + fRecvStart);
	}
		
	//check we have a well-formed message
	if (remaining < kHeaderSize)
		//we don't have enough data for a complete header
	{
		STRACE(("error info: BPortLink remaining %ld bytes is less than header size.\n", remaining));
		ResetReplyBuffer();
		return B_ERROR;
	}
	
	fReplySize = *header;	//size of the first message
	if (fReplySize > remaining || fReplySize < kHeaderSize)
		//the header info declares more data than we have OR
		//the header info declares less data than kHeaderSize
	{
		STRACE(("error info: BPortLink message size of %ld bytes smaller than header size.\n", fReplySize));
		ResetReplyBuffer();
		return B_ERROR;
	}

	*code = *(++header);
	fRecvPosition += kHeaderSize;	//size + code + flags

	STRACE(("info: BPortLink got header %s [%ld %ld %ld] from port %ld.\n", strcode(*code), fReplySize, *code, *(header + 1), fReceivePort));

	return B_OK;
}

void BPortLink::ResetReplyBuffer()
{
	fRecvPosition = 0;
	fRecvStart = 0;
	fDataSize = 0;
	fReplySize = 0;
}

status_t BPortLink::AdjustReplyBuffer(bigtime_t timeout)
{
	//Here we take advantage of the compiler's dead-code elimination
	if (kInitialReceiveBufferSize == kMaxReceiveBufferSize) //fixed buffer size
	{
		if (fRecvBuffer != NULL)
			return B_OK;

		fRecvBuffer = (char *)malloc(kInitialReceiveBufferSize);
		if (fRecvBuffer == NULL)
			return B_NO_MEMORY;
		fRecvBufferSize = kInitialReceiveBufferSize;
	}
	else //if (kInitialReceiveBufferSize < kMaxReceiveBufferSize)
	{
		STRACE(("info: BPortLink getting port_buffer_size().\n"));
		ssize_t buffersize;
		if (timeout == B_INFINITE_TIMEOUT)
			buffersize = port_buffer_size(fReceivePort);
		else
			buffersize = port_buffer_size_etc(fReceivePort, B_TIMEOUT, timeout);
		STRACE(("info: BPortLink got port_buffer_size() = %ld.\n", buffersize));

		if (buffersize < 0)
			return (status_t)buffersize;

		//make sure our receive buffer is large enough
		if (buffersize > fRecvBufferSize)
		{
			if (buffersize <= kInitialReceiveBufferSize)
				buffersize = kInitialReceiveBufferSize;
			else
				buffersize = (buffersize + B_PAGE_SIZE) - (buffersize % B_PAGE_SIZE);
			if (buffersize > kMaxReceiveBufferSize)
				return B_ERROR;	//we can't continue

			STRACE(("info: BPortLink setting receive buffersize to %ld.\n", buffersize));
			char *buffer = (char *)malloc(buffersize);
			if (buffer == NULL)
				return B_NO_MEMORY;
			if (fRecvBuffer)
				free(fRecvBuffer);
			fRecvBuffer = buffer;
			fRecvBufferSize = buffersize;
		}
	}

	return B_OK;
}

status_t BPortLink::ReadFromPort(bigtime_t timeout)
{
	//we are here so it means we finished reading the buffer contents
	ResetReplyBuffer();

	status_t err = AdjustReplyBuffer(timeout);
	if (err < B_OK)
		return err;

	int32 protocol;
	ssize_t bytesread;	
	STRACE(("info: BPortLink reading port %ld.\n", fReceivePort));
	if (timeout != B_INFINITE_TIMEOUT)
	{
		do {
			bytesread = read_port_etc(fReceivePort, &protocol, fRecvBuffer,
				fRecvBufferSize, B_TIMEOUT, timeout);
		} while(bytesread == B_INTERRUPTED);
	}
	else
	{
		do {
			bytesread = read_port(fReceivePort, &protocol, fRecvBuffer, 
				fRecvBufferSize);
		} while(bytesread == B_INTERRUPTED);
	}

	STRACE(("info: BPortLink read %ld bytes.\n", bytesread));
	if (bytesread < B_OK)
		return bytesread;

	//TODO: we only need AS_SERVER_PORTLINK when all OBOS uses BPortLink
	if (protocol != AS_SERVER_PORTLINK && protocol != AS_SERVER_SESSION)
		return B_ERROR;
	if (protocol == AS_SERVER_PORTLINK && bytesread != *((int32 *)fRecvBuffer))
		//should only be one message for PORTLINK so the size declared in the header
		//(the first int32 in the header) should be the same as bytesread
		return B_ERROR;

	fDataSize = bytesread;
	return B_OK;
}

status_t BPortLink::Read(void *data, ssize_t size)
{
//	STRACE(("info: BPortLink Read()ing %ld bytes...\n", size));
	if (fReadError < B_OK)
		return fReadError;

	if (size < 1)
	{
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	if (fDataSize == 0 || fReplySize == 0)
		return B_NO_INIT;	//need to call GetNextReply() first

	if (fRecvPosition + size > fRecvStart + fReplySize)
	{
		//reading past the end of current message
		fReadError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	memcpy(data, fRecvBuffer + fRecvPosition, size);
	fRecvPosition += size;
	return fReadError;
}

status_t BPortLink::ReadString(char **string)
{
	status_t err;
	int32 len = 0;

	err = Read<int32>(&len);
	if (err < B_OK)
		return err;

	if (len)
	{
		*string = (char *)malloc(len);
		if (*string == NULL)
		{
			fRecvPosition -= sizeof(int32);	//rewind the transaction
			return B_NO_MEMORY;
		}

		err = Read(*string, len);
		if (err < B_OK)
		{
			free(*string);
			*string = NULL;
			fRecvPosition -= sizeof(int32);	//rewind the transaction
			return err;
		}
		(*string)[len-1] = '\0';
		return B_OK;
	}
	else
	{
		fRecvPosition -= sizeof(int32);	//rewind the transaction
		return B_ERROR;
	}
}

status_t BPortLink::AttachString(const char *string)
{
	status_t err;
	if (string == NULL)
		return B_BAD_VALUE;

	int32 len = strlen(string)+1;
	err = Attach<int32>(len);
	if (err < B_OK)
		return err;

	err = Attach(string, len);
	if (err < B_OK)
		fSendPosition -= sizeof(int32);	//rewind the transaction
	
	return err;
}

#ifdef DEBUG_BPORTLINK
#include <ServerProtocol.h>

static const char *kASCodeNames[] =
{
"SERVER_TRUE",
"SERVER_FALSE",
"AS_SERVER_BMESSAGE",
"AS_SERVER_AREALINK",
"AS_SERVER_SESSION",
"AS_SERVER_PORTLINK",
"AS_CLIENT_DEAD",
"AS_CREATE_APP",
"AS_DELETE_APP",
"AS_QUIT_APP",
"AS_SET_SERVER_PORT",
"AS_CREATE_WINDOW",
"AS_DELETE_WINDOW",
"AS_CREATE_BITMAP",
"AS_DELETE_BITMAP",
"AS_SET_CURSOR_DATA",	
"AS_SET_CURSOR_BCURSOR",
"AS_SET_CURSOR_BBITMAP",
"AS_SET_CURSOR_SYSTEM",
"AS_SET_SYSCURSOR_DATA",
"AS_SET_SYSCURSOR_BCURSOR",
"AS_SET_SYSCURSOR_BBITMAP",
"AS_SET_SYSCURSOR_DEFAULTS",
"AS_GET_SYSCURSOR",
"AS_SHOW_CURSOR",
"AS_HIDE_CURSOR",
"AS_OBSCURE_CURSOR",
"AS_QUERY_CURSOR_HIDDEN",
"AS_CREATE_BCURSOR",
"AS_DELETE_BCURSOR",
"AS_BEGIN_RECT_TRACKING",
"AS_END_RECT_TRACKING",
"AS_SHOW_WINDOW",
"AS_HIDE_WINDOW",
"AS_QUIT_WINDOW",
"AS_SEND_BEHIND",
"AS_SET_LOOK",
"AS_SET_FEEL", 
"AS_SET_FLAGS",
"AS_DISABLE_UPDATES",
"AS_ENABLE_UPDATES",
"AS_BEGIN_UPDATE",
"AS_END_UPDATE",
"AS_NEEDS_UPDATE",
"AS_WINDOW_TITLE",
"AS_ADD_TO_SUBSET",
"AS_REM_FROM_SUBSET",
"AS_SET_ALIGNMENT",
"AS_GET_ALIGNMENT",
"AS_GET_WORKSPACES",
"AS_SET_WORKSPACES",
"AS_WINDOW_RESIZE",
"AS_WINDOW_MOVE",
"AS_SET_SIZE_LIMITS",
"AS_ACTIVATE_WINDOW",
"AS_WINDOW_MINIMIZE",
"AS_UPDATE_IF_NEEDED",
"_ALL_UPDATED_",
"AS_CREATE_PICTURE",
"AS_DELETE_PICTURE",
"AS_CLONE_PICTURE",
"AS_DOWNLOAD_PICTURE",
"AS_QUERY_FONTS_CHANGED",
"AS_UPDATED_CLIENT_FONTLIST",
"AS_GET_FAMILY_ID",
"AS_GET_STYLE_ID",
"AS_GET_STYLE_FOR_FACE",
"AS_GET_SCREEN_MODE",
"AS_SET_UI_COLORS",
"AS_GET_UI_COLORS",
"AS_GET_UI_COLOR",
"AS_SET_DECORATOR",
"AS_GET_DECORATOR",
"AS_R5_SET_DECORATOR",
"AS_COUNT_WORKSPACES",
"AS_SET_WORKSPACE_COUNT",
"AS_CURRENT_WORKSPACE",
"AS_ACTIVATE_WORKSPACE",
"AS_SET_SCREEN_MODE",
"AS_GET_SCROLLBAR_INFO",
"AS_SET_SCROLLBAR_INFO",
"AS_IDLE_TIME",
"AS_SELECT_PRINTER_PANEL",
"AS_ADD_PRINTER_PANEL",
"AS_RUN_BE_ABOUT",
"AS_SET_FOCUS_FOLLOWS_MOUSE",
"AS_FOCUS_FOLLOWS_MOUSE",
"AS_SET_MOUSE_MODE",
"AS_GET_MOUSE_MODE",
"AS_WORKSPACE_ACTIVATED",
"AS_WORKSPACES_CHANGED",
"AS_WINDOW_ACTIVATED",
"AS_SCREENMODE_CHANGED",
"AS_BEGIN_TRANSACTION",
"AS_END_TRANSACTION",
"AS_SET_HIGH_COLOR",
"AS_SET_LOW_COLOR",
"AS_SET_VIEW_COLOR",
"AS_STROKE_ARC",
"AS_STROKE_BEZIER",
"AS_STROKE_ELLIPSE",
"AS_STROKE_LINE",
"AS_STROKE_LINEARRAY",
"AS_STROKE_POLYGON",
"AS_STROKE_RECT",
"AS_STROKE_ROUNDRECT",
"AS_STROKE_SHAPE",
"AS_STROKE_TRIANGLE",
"AS_FILL_ARC",
"AS_FILL_BEZIER",
"AS_FILL_ELLIPSE",
"AS_FILL_POLYGON",
"AS_FILL_RECT",
"AS_FILL_REGION",
"AS_FILL_ROUNDRECT",
"AS_FILL_SHAPE",
"AS_FILL_TRIANGLE",
"AS_MOVEPENBY",
"AS_MOVEPENTO",
"AS_SETPENSIZE",
"AS_DRAW_STRING",
"AS_SET_FONT",
"AS_SET_FONT_SIZE",
"AS_FLUSH",
"AS_SYNC",
"AS_LAYER_CREATE",
"AS_LAYER_DELETE",
"AS_LAYER_CREATE_ROOT",
"AS_LAYER_DELETE_ROOT",
"AS_LAYER_ADD_CHILD", 
"AS_LAYER_REMOVE_CHILD",
"AS_LAYER_REMOVE_SELF",
"AS_LAYER_SHOW",
"AS_LAYER_HIDE",
"AS_LAYER_MOVE",
"AS_LAYER_RESIZE",
"AS_LAYER_INVALIDATE",
"AS_LAYER_DRAW",
"AS_LAYER_GET_TOKEN",
"AS_LAYER_ADD",
"AS_LAYER_REMOVE",
"AS_LAYER_GET_COORD",
"AS_LAYER_SET_FLAGS",
"AS_LAYER_SET_ORIGIN",
"AS_LAYER_GET_ORIGIN",
"AS_LAYER_RESIZE_MODE",
"AS_LAYER_CURSOR",
"AS_LAYER_BEGIN_RECT_TRACK",
"AS_LAYER_END_RECT_TRACK",
"AS_LAYER_DRAG_RECT",
"AS_LAYER_DRAG_IMAGE",
"AS_LAYER_GET_MOUSE_COORDS",
"AS_LAYER_SCROLL",
"AS_LAYER_SET_LINE_MODE",
"AS_LAYER_GET_LINE_MODE",
"AS_LAYER_PUSH_STATE",
"AS_LAYER_POP_STATE",
"AS_LAYER_SET_SCALE",
"AS_LAYER_GET_SCALE",
"AS_LAYER_SET_DRAW_MODE",
"AS_LAYER_GET_DRAW_MODE",
"AS_LAYER_SET_BLEND_MODE",
"AS_LAYER_GET_BLEND_MODE",
"AS_LAYER_SET_PEN_LOC",
"AS_LAYER_GET_PEN_LOC",
"AS_LAYER_SET_PEN_SIZE",
"AS_LAYER_GET_PEN_SIZE",
"AS_LAYER_SET_HIGH_COLOR",
"AS_LAYER_SET_LOW_COLOR",
"AS_LAYER_SET_VIEW_COLOR",
"AS_LAYER_GET_COLORS",
"AS_LAYER_PRINT_ALIASING",
"AS_LAYER_CLIP_TO_PICTURE",
"AS_LAYER_CLIP_TO_INVERSE_PICTURE",
"AS_LAYER_GET_CLIP_REGION",
"AS_LAYER_DRAW_BITMAP_ASYNC_IN_RECT",
"AS_LAYER_DRAW_BITMAP_ASYNC_AT_POINT",
"AS_LAYER_DRAW_BITMAP_SYNC_IN_RECT",
"AS_LAYER_DRAW_BITMAP_SYNC_AT_POINT",
"AS_LAYER_DRAW_STRING",
"AS_LAYER_SET_CLIP_REGION",
"AS_LAYER_LINE_ARRAY",
"AS_LAYER_BEGIN_PICTURE",
"AS_LAYER_APPEND_TO_PICTURE",
"AS_LAYER_END_PICTURE",
"AS_LAYER_COPY_BITS",
"AS_LAYER_DRAW_PICTURE",
"AS_LAYER_INVAL_RECT",
"AS_LAYER_INVAL_REGION",
"AS_LAYER_INVERT_RECT",
"AS_LAYER_MOVETO",
"AS_LAYER_RESIZETO",
"AS_LAYER_SET_STATE",
"AS_LAYER_SET_FONT_STATE",
"AS_LAYER_GET_STATE",
"AS_LAYER_SET_VIEW_IMAGE",
"AS_LAYER_SET_PATTERN",
"AS_SET_CURRENT_LAYER",
};

const char *strcode(int32 code)
{
	code = code - SERVER_TRUE;
	if (code >= 0 && code <= AS_SET_CURRENT_LAYER - SERVER_TRUE)
		return kASCodeNames[code];
	else
		return "Unknown";
}
#endif //DEBUG_BPORTLINK


