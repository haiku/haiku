// ClipboardHandler.cpp

#include <Message.h>

#include "ClipboardHandler.h"

/*!
	\class ClipboardHandler
	\brief Handles all clipboard related requests.
*/

// constructor
/*!	\brief Creates and initializes a ClipboardHandler.
*/
ClipboardHandler::ClipboardHandler()
				: BHandler()
{
}

// destructor
/*!	\brief Frees all resources associate with this object.
*/
ClipboardHandler::~ClipboardHandler()
{
}

// MessageReceived
/*!	\brief Overrides the super class version to handle the clipboard specific
		   messages.
	\param message The message to be handled
*/
void
ClipboardHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BHandler::MessageReceived(message);
			break;
	}
}

