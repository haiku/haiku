/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "VMWareMouse.h"
#include "VMWareTypes.h"

#include <Message.h>
#include <Screen.h>

#include <new>
#include <syslog.h>


#define DEBUG_PREFIX		"VMWareMouseFilter: "
#define TRACE(x...)			/*syslog(DEBUG_PREFIX x)*/
#define TRACE_ALWAYS(x...)	syslog(LOG_INFO, DEBUG_PREFIX x)
#define TRACE_ERROR(x...)	syslog(LOG_ERR, DEBUG_PREFIX x)


// #pragma mark - Public BInputServerFilter API


VMWareMouseFilter::VMWareMouseFilter()
	:
	BInputServerFilter()
{
	fIsEnabled = _Enable();
}


VMWareMouseFilter::~VMWareMouseFilter()
{
	if (fIsEnabled)
		_Disable();
}


status_t
VMWareMouseFilter::InitCheck()
{
	if (fIsEnabled)
		return B_OK;
	return B_ERROR;
}


filter_result
VMWareMouseFilter::Filter(BMessage *message, BList *outList)
{
	if (!fIsEnabled)
		return B_DISPATCH_MESSAGE;

	switch(message->what) {
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
		case B_MOUSE_MOVED:
		{
			uint16 status, numWords;
			_GetStatus(&status, &numWords);
			if (status == VMWARE_ERROR) {
				TRACE_ERROR("error indicated when reading status, resetting\n");
				_Disable();
				fIsEnabled = _Enable();
				break;
			}

			if (numWords == 0) {
				// no actual data availabe, spurious event happens on fast move
				return B_SKIP_MESSAGE;
			}

			int32 x, y;
			_GetPosition(x, y);
			_ScalePosition(x, y);

			if (x < 0 || y < 0) {
				TRACE_ERROR("got invalid coordinates %ld, %ld\n", x, y);
				break;
			}

			TRACE("setting position to %ld, %ld\n", x, y);
			message->RemoveName("where");
			message->AddPoint("where", BPoint(x, y));
			break;
		}
	}

	return B_DISPATCH_MESSAGE;
}


// #pragma mark - VMWare Communication


void
VMWareMouseFilter::_ExecuteCommand(union packet_u &packet)
{
	packet.command.magic = VMWARE_PORT_MAGIC;
	packet.command.port = VMWARE_PORT_NUMBER;

	int dummy;
	asm volatile (
			"pushl %%ebx;"
			"pushl %%eax;"
			"movl 12(%%eax), %%edx;"
			"movl 8(%%eax), %%ecx;"
			"movl 4(%%eax), %%ebx;"
			"movl (%%eax), %%eax;"
			"inl %%dx, %%eax;"
			"xchgl %%eax, (%%esp);"
			"movl %%edx, 12(%%eax);"
			"movl %%ecx, 8(%%eax);"
			"movl %%ebx, 4(%%eax);"
			"popl (%%eax);"
			"popl %%ebx;"
		: "=a"(dummy)
		: "0"(&packet)
		: "ecx", "edx", "memory");
}


bool
VMWareMouseFilter::_Enable()
{
	union packet_u packet;
	packet.command.command = VMWARE_COMMAND_POINTER_COMMAND;
	packet.command.value = VMWARE_VALUE_READ_ID;
	_ExecuteCommand(packet);

	uint16 numWords;
	_GetStatus(NULL, &numWords);
	if (numWords == 0) {
		TRACE_ERROR("didn't get back data on reading version id\n");
		return false;
	}

	packet.command.command = VMWARE_COMMAND_POINTER_DATA;
	packet.command.value = 1; // read size, 1 word
	_ExecuteCommand(packet);

	if (packet.version.version != VMWARE_VERSION_ID) {
		TRACE_ERROR("got back unexpected version 0x%08lx\n",
			packet.version.version);
		return false;
	}

	// request absolute data
	packet.command.command = VMWARE_COMMAND_POINTER_COMMAND;
	packet.command.value = VMWARE_VALUE_REQUEST_ABSOLUTE;
	_ExecuteCommand(packet);

	TRACE_ALWAYS("successfully enabled\n");
	return true;
}


void
VMWareMouseFilter::_Disable()
{
	union packet_u packet;
	packet.command.command = VMWARE_COMMAND_POINTER_COMMAND;
	packet.command.value = VMWARE_VALUE_DISABLE;
	_ExecuteCommand(packet);

	uint16 status;
	_GetStatus(&status, NULL);
	if (status != VMWARE_ERROR) {
		TRACE_ERROR("didn't get expected status value after disabling\n");
		return;
	}

	TRACE_ALWAYS("successfully disabled\n");
}


void
VMWareMouseFilter::_GetStatus(uint16 *status, uint16 *numWords)
{
	union packet_u packet;
	packet.command.command = VMWARE_COMMAND_POINTER_STATUS;
	packet.command.value = 0;
	_ExecuteCommand(packet);

	if (status != NULL)
		*status = packet.status.status;
	if (numWords != NULL)
		*numWords = packet.status.num_words;
}


void
VMWareMouseFilter::_GetPosition(int32 &x, int32 &y)
{
	union packet_u packet;
	packet.command.command = VMWARE_COMMAND_POINTER_DATA;
	packet.command.value = 4; // read size, 4 words
	_ExecuteCommand(packet);
	x = packet.data.x;
	y = packet.data.y;
}


void
VMWareMouseFilter::_ScalePosition(int32 &x, int32 &y)
{
	static BScreen screen;
	BRect frame = screen.Frame();
	x = (int32)(x * (frame.Width() / 65535) + 0.5);
	y = (int32)(y * (frame.Height() / 65535) + 0.5);
}


// #pragma mark - Instatiation Entry Point


extern "C"
BInputServerFilter *instantiate_input_filter()
{
	return new (std::nothrow) VMWareMouseFilter();
}
