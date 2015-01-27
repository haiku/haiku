/*
 * Copyright 2009-2015 Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		John Scipione, jscipione@gmail.com
 */


#include "SwitchWorkspaceInputFilter.h"

#include <string.h>

#include <new>

#include <AppServerLink.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <ServerProtocol.h>


#define _SWITCH_WORKSPACE_	'_SWS'


extern "C" BInputServerFilter* instantiate_input_filter() {
	return new(std::nothrow) SwitchWorkspaceInputFilter();
}


SwitchWorkspaceInputFilter::SwitchWorkspaceInputFilter()
{
}


filter_result
SwitchWorkspaceInputFilter::Filter(BMessage* message, BList* _list)
{
	switch (message->what) {
		case B_KEY_DOWN:
		{
			const char* bytes;
			if (message->FindString("bytes", &bytes) != B_OK)
				break;

			int32 modifiers;
			if (message->FindInt32("modifiers", &modifiers) != B_OK)
				break;

			int32 modifiersHeld = modifiers & (B_COMMAND_KEY
				| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY | B_SHIFT_KEY);

			bool takeMeThere;
			if (modifiersHeld == (B_COMMAND_KEY | B_CONTROL_KEY))
				takeMeThere = false;
			else if (modifiersHeld
					== (B_COMMAND_KEY | B_CONTROL_KEY | B_SHIFT_KEY)) {
				takeMeThere = true;
			} else
				break;

			int32 deltaX = 0;
			int32 deltaY = 0;

			if (bytes[0] == B_LEFT_ARROW)
				deltaX = -1;
			else if (bytes[0] == B_UP_ARROW)
				deltaY = -1;
			else if (bytes[0] == B_RIGHT_ARROW)
				deltaX = 1;
			else if (bytes[0] == B_DOWN_ARROW)
				deltaY = 1;
			else
				break;

			BPrivate::AppServerLink link;
			link.StartMessage(AS_GET_WORKSPACE_LAYOUT);

			status_t status;
			int32 columns;
			int32 rows;
			if (link.FlushWithReply(status) != B_OK || status != B_OK)
				break;

			link.Read<int32>(&columns);
			link.Read<int32>(&rows);

			int32 current = current_workspace();

			int32 nextColumn = current % columns + deltaX;
			if (nextColumn >= columns)
				nextColumn = columns - 1;
			else if (nextColumn < 0)
				nextColumn = 0;

			int32 nextRow = current / columns + deltaY;
			if (nextRow >= rows)
				nextRow = rows - 1;
			else if (nextRow < 0)
				nextRow = 0;

			int32 next = nextColumn + nextRow * columns;
			if (next != current) {
				BPrivate::AppServerLink link;
				link.StartMessage(AS_ACTIVATE_WORKSPACE);
				link.Attach<int32>(next);
				link.Attach<bool>(takeMeThere);
				link.Flush();
			}

			return B_SKIP_MESSAGE;
		}
	}

	return B_DISPATCH_MESSAGE;
}


status_t
SwitchWorkspaceInputFilter::InitCheck()
{
	return B_OK;
}
