//------------------------------------------------------------------------------
//	Copyright (c) 2005, Haiku, Inc.
//	Distributed under the terms of the MIT license.
//
//	File Name:		fake_input_server.h
//	Authors:		Stephan Aßmus <superstippi@gmx.de>
//	Description:	prototypes for the fake input_server communication
//  
//------------------------------------------------------------------------------

#ifndef FAKE_INPUT_SERVER_H
#define FAKE_INPUT_SERVER_H

#include <Point.h>

class BPortLink;
class BMessage;

void
send_mouse_down(BPortLink* serverLink, BPoint pt,
				BMessage* currentMessage);

void
send_mouse_moved(BPortLink* serverLink, BPoint pt,
				 BMessage* currentMessage);

void
send_mouse_up(BPortLink* serverLink, BPoint pt,
			  BMessage* currentMessage);

bool
handle_message(BPortLink* serverLink, BMessage* msg);

#endif	// FAKE_INPUT_SERVER_H
