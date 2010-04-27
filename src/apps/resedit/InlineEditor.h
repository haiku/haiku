/*
 * Copyright (c) 2005-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@gmail.com>
 */
#ifndef INLINE_EDITOR
#define INLINE_EDITOR

#include <Messenger.h>
#include <TextControl.h>
#include <Window.h>

#define M_INLINE_TEXT 'intx'

class InlineEditor : public BWindow
{
public:
			InlineEditor(BMessenger target, const BRect &frame,
						const char *text);
	bool	QuitRequested(void);
	void	SetMessage(BMessage *msg);
	void	MessageReceived(BMessage *msg);
	void	WindowActivated(bool active);
	
private:
	BTextControl	*fTextBox;
	BMessenger		fMessenger;
	uint32			fCommand;
};


#endif
