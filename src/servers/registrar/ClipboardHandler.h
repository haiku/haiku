// ClipboardHandler.h

#ifndef CLIPBOARD_HANDLER_H
#define CLIPBOARD_HANDLER_H

#include <Handler.h>

class ClipboardHandler : public BHandler {
public:
	ClipboardHandler();
	virtual ~ClipboardHandler();

	virtual void MessageReceived(BMessage *message);
};

#endif	// CLIPBOARD_HANDLER_H
