// ClipboardHandler.h

#ifndef CLIPBOARD_HANDLER_H
#define CLIPBOARD_HANDLER_H

#include <Handler.h>
#include <Message.h>

class Clipboard;

class ClipboardHandler : public BHandler {
public:
	ClipboardHandler();
	virtual ~ClipboardHandler();

	virtual void MessageReceived(BMessage *message);

private:
	Clipboard *_GetClipboard(const char *name);

	struct ClipboardMap;

	ClipboardMap	*fClipboards;
};

#endif	// CLIPBOARD_HANDLER_H

