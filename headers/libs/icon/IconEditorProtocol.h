/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */

#ifndef ICON_EDITOR_PROTOCOL_H
#define ICON_EDITOR_PROTOCOL_H

enum {
	B_EDIT_ICON_DATA	= 'EICN',
	// the message needs to contain these fields:
	// B_MESSENGER_TYPE		"reply to"		the messenger to which
	//										B_ICON_DATA_EDITED messages should
	//										be sent
	// B_VECTOR_ICON_TYPE	"icon data"		OPTIONAL - the original vector icon data
	//										- if missing, a new icon is created

	B_ICON_DATA_EDITED	= 'ICNE',
	// the message needs to contain these fields:
	// B_VECTOR_ICON_TYPE	"icon data"		the changed icon data, do with it
	//										as you please, the message is sent
	//										to the specified messenger and is
	//										the result of the user having
	//										decided to "save" the edited icon
};

#endif // ICON_EDITOR_PROTOCOL_H
