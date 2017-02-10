//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------

#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

class BMessage;

class MessageHandler {
public:
	MessageHandler();
	virtual ~MessageHandler();

	virtual void HandleMessage(BMessage *message);
};

#endif	// MESSAGE_HANDLER_H
