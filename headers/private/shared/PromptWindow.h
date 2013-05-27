/*
 * Copyright 2012-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef PROMPT_WINDOW_H_
#define PROMPT_WINDOW_H_


#include <Messenger.h>
#include <Window.h>


class BStringView;
class BTextControl;


class PromptWindow : public BWindow
{
public:
								// PromptWindow takes ownership of message
								PromptWindow(const char* title,
									const char* label, const char* info,
									BMessenger target, BMessage* message = NULL);
								~PromptWindow();

	virtual void				MessageReceived(BMessage* message);

		status_t				SetTarget(BMessenger messenger);
		status_t				SetMessage(BMessage* message);
private:
		BTextControl*			fTextControl;
		BStringView*			fInfoView;
		BMessenger				fTarget;
		BMessage*				fMessage;
};

#endif // PROMPT_WINDOW_H_
