/*
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROMPT_WINDOW_H_
#define PROMPT_WINDOW_H_


#include <Messenger.h>
#include <Window.h>


class BTextControl;


class PromptWindow : public BWindow
{
public:
								// PromptWindow takes ownership of message
								PromptWindow(const char* title, const char* label, BMessenger target,
									BMessage* message = NULL);
								~PromptWindow();

	virtual void				MessageReceived(BMessage* message);

		status_t				SetTarget(BMessenger messenger);
		status_t				SetMessage(BMessage* message);
private:
		BTextControl*			fTextControl;
		BMessenger				fTarget;
		BMessage*				fMessage;
};

#endif // PROMPT_WINDOW_H_
