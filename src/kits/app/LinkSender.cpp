/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/** Class for low-overhead port-based messaging */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

#include <ServerProtocol.h>
#include <LinkMsgSender.h>

#include "link_message.h"


//#define DEBUG_BPORTLINK
#ifdef DEBUG_BPORTLINK
#	include <stdio.h>
#	define STRACE(x) printf x
	const char *strcode(int32 code);
	const char *bstrcode(int32 code);
#else
#	define STRACE(x) ;
#endif

static const size_t kWatermark = kInitialBufferSize - 24;
	// if a message is started after this mark, the buffer is flushed automatically

namespace BPrivate {

LinkSender::LinkSender(port_id port)
	:
	fPort(port),
	fBuffer(NULL),
	fBufferSize(0),

	fCurrentEnd(0),
	fCurrentStart(0),
	fCurrentStatus(B_OK)
{
}


LinkSender::~LinkSender()
{
	free(fBuffer);
}


void
LinkSender::SetPort(port_id port)
{
	fPort = port;
}


status_t
LinkSender::StartMessage(int32 code, size_t minSize)
{
	// end previous message
	if (EndMessage() < B_OK)	
		CancelMessage();

	if (minSize > kMaxBufferSize - sizeof(message_header))
		return fCurrentStatus = B_BUFFER_OVERFLOW;

	minSize += sizeof(message_header);

	// Eventually flush buffer to make space for the new message.
	// Note, we do not take the actual buffer size into account to not
	// delay the time between buffer flushes too much.
	if (fBufferSize > 0 && (minSize > SpaceLeft() || fCurrentStart >= kWatermark)) {
		status_t status = Flush();
		if (status < B_OK)
			return status;
	}

	if (minSize > fBufferSize) {
		if (AdjustBuffer(minSize) != B_OK)
			return fCurrentStatus = B_NO_MEMORY;
	}

	message_header *header = (message_header *)(fBuffer + fCurrentStart);
	header->size = 0;
		// will be set later
	header->code = code;
	header->flags = 0;

	STRACE(("info: LinkSender buffered header %s (%lx) [%lu %lu %lu].\n",
		strcode(code), code, header->size, header->code, header->flags));

	fCurrentEnd += sizeof(message_header);
	return B_OK;
}


status_t
LinkSender::EndMessage(bool needsReply)
{
	if (fCurrentEnd == fCurrentStart || fCurrentStatus < B_OK)
		return fCurrentStatus;

	// record the size of the message
	message_header *header = (message_header *)(fBuffer + fCurrentStart);
	header->size = CurrentMessageSize();
	if (needsReply)
		header->flags |= needsReply;

	STRACE(("info: LinkSender EndMessage() of size %ld.\n", header->size));

	// bump to start of next message
	fCurrentStart = fCurrentEnd;	
	return B_OK;
}


void
LinkSender::CancelMessage()
{
	fCurrentEnd = fCurrentStart;
	fCurrentStatus = B_OK;
}


status_t
LinkSender::Attach(const void *data, size_t size)
{
	if (fCurrentStatus < B_OK)
		return fCurrentStatus;

	if (size == 0)
		return fCurrentStatus = B_BAD_VALUE;

	if (fCurrentEnd == fCurrentStart)
		return B_NO_INIT;	// need to call StartMessage() first

	if (SpaceLeft() < size) {
		// we have to make space for the data

		status_t status = FlushCompleted(size + CurrentMessageSize());
		if (status < B_OK)
			return fCurrentStatus = status;
	}

	memcpy(fBuffer + fCurrentEnd, data, size);
	fCurrentEnd += size;

	return B_OK;
}


status_t
LinkSender::AttachString(const char *string, int32 length)
{
	if (string == NULL)
		string = "";

	if (length == -1)
		length = strlen(string);

	status_t status = Attach<int32>(length);
	if (status < B_OK)
		return status;

	if (length > 0) {
		status = Attach(string, length);
		if (status < B_OK)
			fCurrentEnd -= sizeof(int32);	// rewind the transaction
	}

	return status;
}


status_t
LinkSender::AdjustBuffer(size_t newSize, char **_oldBuffer)
{
	// make sure the new size is within bounds
	if (newSize <= kInitialBufferSize)
		newSize = kInitialBufferSize;
	else if (newSize > kMaxBufferSize)
		return B_BUFFER_OVERFLOW;
	else if (newSize > kInitialBufferSize)
		newSize = (newSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	char *buffer = NULL;
	if (newSize == fBufferSize) {
		// keep existing buffer
		if (_oldBuffer)
			*_oldBuffer = fBuffer;
		return B_OK;
	}

	// create new larger buffer
	buffer = (char *)malloc(newSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (_oldBuffer)
		*_oldBuffer = fBuffer;
	else
		free(fBuffer);

	fBuffer = buffer;
	fBufferSize = newSize;
	return B_OK;
}


status_t
LinkSender::FlushCompleted(size_t newBufferSize)
{
	// we need to hide the incomplete message so that it's not flushed
	int32 end = fCurrentEnd;
	int32 start = fCurrentStart;
	fCurrentEnd = fCurrentStart;

	status_t status = Flush();
	if (status < B_OK) {
		fCurrentEnd = end;
		return status;
	}

	char *oldBuffer = NULL;
	status = AdjustBuffer(newBufferSize, &oldBuffer);
	if (status != B_OK)
		return status;

	// move the incomplete message to the start of the buffer
	fCurrentEnd = end - start;
	if (oldBuffer != fBuffer) {
		memcpy(fBuffer, oldBuffer + start, fCurrentEnd);
		free(oldBuffer);
	} else
		memmove(fBuffer, fBuffer + start, fCurrentEnd);

	return B_OK;
}


status_t
LinkSender::Flush(bigtime_t timeout, bool needsReply)
{
	if (fCurrentStatus < B_OK)
		return fCurrentStatus;

	EndMessage(needsReply);
	if (fCurrentStart == 0)
		return B_OK;

	STRACE(("info: LinkSender Flush() waiting to send messages of %ld bytes on port %ld.\n",
		fCurrentEnd, fPort));

	status_t err;
	if (timeout != B_INFINITE_TIMEOUT) {
		do {
			err = write_port_etc(fPort, kLinkCode, fBuffer,
				fCurrentEnd, B_RELATIVE_TIMEOUT, timeout);
		} while (err == B_INTERRUPTED);
	} else {
		do {
			err = write_port(fPort, kLinkCode, fBuffer, fCurrentEnd);
		} while (err == B_INTERRUPTED);
	}

	if (err < B_OK) {
		STRACE(("error info: LinkSender Flush() failed for %ld bytes (%s) on port %ld.\n",
			fCurrentEnd, strerror(err), fPort));
		return err;
	}

	STRACE(("info: LinkSender Flush() messages total of %ld bytes on port %ld.\n",
		fCurrentEnd, fPort));

	fCurrentEnd = 0;
	fCurrentStart = 0;

	return B_OK;
}

}	// namespace BPrivate

//	#pragma mark -


#ifdef DEBUG_BPORTLINK
#include <ServerProtocol.h>
#include <AppDefs.h>

static const char *kASCodeNames[] = {
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
	"AS_ACTIVATE_APP",
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

const char *
strcode(int32 code)
{
	code = code - SERVER_TRUE;
	if (code >= 0 && code <= AS_SET_CURRENT_LAYER - SERVER_TRUE)
		return kASCodeNames[code];

	return bstrcode(code);
}


const char *
bstrcode(int32 code)
{
	switch (code) {
		case B_ABOUT_REQUESTED:
			return "B_ABOUT_REQUESTED";
		case B_APP_ACTIVATED:
			return "B_APP_ACTIVATED/B_WINDOW_ACTIVATED";
		case B_ARGV_RECEIVED :
			return "B_ARGV_RECEIVED";
		case B_QUIT_REQUESTED :
			return "B_QUIT_REQUESTED";
		case B_CANCEL:
			return "B_CANCEL";
		case B_KEY_DOWN :
			return "B_KEY_DOWN";
		case B_KEY_UP :
			return "B_KEY_UP";
		case B_UNMAPPED_KEY_DOWN :
			return "B_UNMAPPED_KEY_DOWN";
		case B_UNMAPPED_KEY_UP :
			return "B_UNMAPPED_KEY_UP";
		case B_MODIFIERS_CHANGED:
			return "B_MODIFIERS_CHANGED";
		case B_MINIMIZE:
			return "B_MINIMIZE";
		case B_MOUSE_DOWN :
			return "B_MOUSE_DOWN";
		case B_MOUSE_MOVED :
			return "B_MOUSE_MOVED";
		case B_MOUSE_ENTER_EXIT:
			return "B_MOUSE_ENTER_EXIT";
		case B_MOUSE_UP :
			return "B_MOUSE_UP";
		case B_MOUSE_WHEEL_CHANGED:
			return "B_MOUSE_WHEEL_CHANGED";
		case B_OPEN_IN_WORKSPACE:
			return "B_OPEN_IN_WORKSPACE";
		case B_PRINTER_CHANGED:
			return "B_PRINTER_CHANGED";
		case B_PULSE :
			return "B_PULSE";
		case B_READY_TO_RUN :
			return "B_READY_TO_RUN";
		case B_REFS_RECEIVED :
			return "B_REFS_RECEIVED";
		case B_RELEASE_OVERLAY_LOCK:
			return "B_RELEASE_OVERLAY_LOCK";
		case B_ACQUIRE_OVERLAY_LOCK:
			return "B_ACQUIRE_OVERLAY_LOCK";
		case B_SCREEN_CHANGED :
			return "B_SCREEN_CHANGED";
		case B_VALUE_CHANGED :
			return "B_VALUE_CHANGED";
		case B_VIEW_MOVED :
			return "B_VIEW_MOVED";
		case B_VIEW_RESIZED :
			return "B_VIEW_RESIZED";
		case B_WINDOW_MOVED :
			return "B_WINDOW_MOVED";
		case B_WINDOW_RESIZED :
			return "B_WINDOW_RESIZED";
		case B_WORKSPACES_CHANGED:
			return "B_WORKSPACES_CHANGED";
		case B_WORKSPACE_ACTIVATED:
			return "B_WORKSPACE_ACTIVATED";
		case B_ZOOM:
			return "B_ZOOM";
		case _APP_MENU_:
			return "_APP_MENU";
		case _BROWSER_MENUS_:
			return "_BROWSER_MENUS_";
		case _MENU_EVENT_ :
			return "_MENU_EVENT";
		case _PING_:
			return "_PING_";
		case _QUIT_ :
			return "_QUIT_";
		case _VOLUME_MOUNTED_ :
			return "_VOLUME_MOUNTED";
		case _VOLUME_UNMOUNTED_:
			return "_VOLUME_UNMOUNTED";
		case _MESSAGE_DROPPED_ :
			return "_MESSAGE_DROPPED";
		case _DISPOSE_DRAG_ :
			return "_DISPOSE_DRAG";
		case _MENUS_DONE_:
			return "_MENUS_DONE_";
		case _SHOW_DRAG_HANDLES_:
			return "_SHOW_DRAG_HANDLES_";
		case _EVENTS_PENDING_ :
			return "_EVENTS_PENDING_";
		case _UPDATE_ :
			return "_UPDATE_";
		case _UPDATE_IF_NEEDED_:
			return "_UPDATE_IF_NEEDED_";
		case _PRINTER_INFO_:
			return "_PRINTER_INFO_";
		case _SETUP_PRINTER_:
			return "_SETUP_PRINTER_";
		case _SELECT_PRINTER_:
			return "_SELECT_PRINTER_";
		case B_SET_PROPERTY:
			return "B_SET_PROPERTY";
		case B_GET_PROPERTY:
			return "B_GET_PROERTY";
		case B_CREATE_PROPERTY:
			return "B_CREATE_PROPERTY";
		case B_DELETE_PROPERTY:
			return "B_DELETE_PROPERTY";
		case B_COUNT_PROPERTIES:
			return "B_COUNT_PROPERTIES";
		case B_EXECUTE_PROPERTY:
			return "B_EXECUTE_PROPERTY";
		case B_GET_SUPPORTED_SUITES:
			return "B_GET_SUPPORTED_SUITES";
		case B_UNDO:
			return "B_UNDO";
		case B_CUT :
			return "B_CUT";
		case B_COPY :
			return "B_COPY";
		case B_PASTE :
			return "B_PASTE";
		case B_SELECT_ALL:
			return "B_SELECT_ALL";
		case B_SAVE_REQUESTED :
			return "B_SAVE_REQUESTED";
		case B_MESSAGE_NOT_UNDERSTOOD:
			return "B_MESSAGE_NOT_UNDERSTOOD";
		case B_NO_REPLY :
			return "B_NO_REPLY";
		case B_REPLY :
			return "B_REPLY";
		case B_SIMPLE_DATA:
			return "B_SIMPLE_DATA";
		case B_MIME_DATA:
			return "B_MIME_DATA";
		case B_ARCHIVED_OBJECT:
			return "B_ARCHIVED_OBJECT";
		case B_UPDATE_STATUS_BAR:
			return "B_UPDATE_STATUS_BAR";
		case B_RESET_STATUS_BAR:
			return "B_RESET_STATUS_BAR";
		case B_NODE_MONITOR:
			return "B_NODE_MONITOR";
		case B_QUERY_UPDATE:
			return "B_QUERY_UPDATE";
		case B_ENDORSABLE:
			return "B_ENDORSABLE";
		case B_COPY_TARGET:
			return "B_COPY_TARGET";
		case B_MOVE_TARGET:
			return "B_MOVE_TARGET";
		case B_TRASH_TARGET:
			return "B_TRASH_TARGET";
		case B_LINK_TARGET:
			return "B_LINK_TARGET";
		case B_INPUT_DEVICES_CHANGED:
			return "B_INPUT_DEVICES_CHANGED";
		case B_INPUT_METHOD_EVENT:
			return "B_INPUT_METHOD_EVENT";
		case B_WINDOW_MOVE_TO:
			return "B_WINDOW_MOVE_TO";
		case B_WINDOW_MOVE_BY:
			return "B_WINDOW_MOVE_BY";
		case B_SILENT_RELAUNCH:
			return "B_SILENT_RELAUNCH";
		case B_OBSERVER_NOTICE_CHANGE :
			return "B_OBSERVER_NOTICE_CHANGE";
		case B_CONTROL_INVOKED:
			return "B_CONTROL_INVOKED";
		case B_CONTROL_MODIFIED:
			return "B_CONTROL_MODIFIED";
		default:
			return "Unknown";
	}
}
#endif //DEBUG_BPORTLINK

