// ClipboardHandler.h

#ifndef CLIPBOARD_HANDLER_H
#define CLIPBOARD_HANDLER_H

#include <Handler.h>
#include <Message.h>
#include "ClipboardTree.h"

class ClipboardHandler : public BHandler {
public:
	ClipboardHandler();
	virtual ~ClipboardHandler();

	virtual void MessageReceived(BMessage *message);
private:
	ClipboardTree fClipboardTree;
};

#endif	// CLIPBOARD_HANDLER_H

