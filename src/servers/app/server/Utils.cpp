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
//	File Name:		Utils.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Miscellaneous utility functions
//
//------------------------------------------------------------------------------
#include <Entry.h>
#include <stdio.h>
#include "Utils.h"
#include <String.h>

/*!
	\brief Send a BMessage to a Looper target
	\param port Receiver port of the targeted Looper
	\param BMessage to send
	
	This SendMessage takes ownership of the message sent, so deleting them after this 
	call is unnecessary. Passing an invalid port will have unpredictable results.
*/
void SendMessage(port_id port, BMessage *message, int32 target)
{
	if(!message)
		return;

	if(target==-1)
		_set_message_target_(message,target,true);
	else
		_set_message_target_(message,target,false);
	
	ssize_t flatsize=message->FlattenedSize();
	char *buffer=new char[flatsize];
	
	if(message->Flatten(buffer,flatsize)==B_OK)
		write_port(port, message->what, buffer,flatsize);
	
	delete buffer;
	delete message;
}

/*
	Below are friend functions for BMessage which currently are not in the Message.cpp 
	that we need to send messages to BLoopers and such. Placed here to allow compilation.
*/
void _set_message_target_(BMessage *msg, int32 target, bool preferred)
{
	if (preferred)
	{
		msg->fTarget = -1;
		msg->fPreferred = true;
	}
	else
	{
		msg->fTarget = target;
		msg->fPreferred = false;
	}
}

int32 _get_message_target_(BMessage *msg)
{
	if (msg->fPreferred)
		return -1;
	else
		return msg->fTarget;
}

bool _use_preferred_target_(BMessage *msg)
{
	return msg->fPreferred;
}

const char *MsgCodeToString(int32 code)
{
	// Used to translate BMessage message codes back to a character
	// format
	char string [10];
	sprintf(string,"'%x%x%x%x'",(char)((code & 0xFF000000) >>  24),
		(char)((code & 0x00FF0000) >>  16),
		(char)((code & 0x0000FF00) >>  8),
		(char)((code & 0x000000FF)) );
	return string;
}

BString MsgCodeToBString(int32 code)
{
	// Used to translate BMessage message codes back to a character
	// format
	char string [10];
	sprintf(string,"'%x%x%x%x'",(char)((code & 0xFF000000) >>  24),
		(char)((code & 0x00FF0000) >>  16),
		(char)((code & 0x0000FF00) >>  8),
		(char)((code & 0x000000FF)) );

	BString bstring(string);
	return bstring;
}
