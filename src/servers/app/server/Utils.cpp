#include "Utils.h"

/*!
	\brief Send a BMessage to a Looper target
	\param port Receiver port of the targeted Looper
	\param BMessage to send
	
	This SendMessage takes ownership of the message sent, so deleting them after this 
	call is unnecessary. Passing an invalid port will have unpredictable results.
*/
void SendMessage(port_id port, BMessage *message)
{
	if(!message)
		return;
	
	ssize_t flatsize=message->FlattenedSize();
	char *buffer=new char[flatsize];
	
	if(message->Flatten(buffer,flatsize)==B_OK)
		write_port(port, message->what, buffer,flatsize);
	
	delete buffer;
	delete message;
}
