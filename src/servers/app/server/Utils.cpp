#include <Entry.h>
#include <stdio.h>
#include "Utils.h"

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
	sprintf(string,"'%c%c%c%c'",(char)((code & 0xFF000000) >>  24),
		(char)((code & 0x00FF0000) >>  16),
		(char)((code & 0x0000FF00) >>  8),
		(char)((code & 0x000000FF)) );
	return string;
}
