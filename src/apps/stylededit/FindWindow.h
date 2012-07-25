/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef FIND_WINDOW_H 
#define FIND_WINDOW_H 


#include <Window.h>


class BButton;
class BCheckBox;
class BTextControl;


class FindWindow : public BWindow {
	public:
						FindWindow(BRect frame, BHandler* handler,
							BString* searchString, bool caseState,
							bool wrapState, bool backState);

		virtual void	MessageReceived(BMessage* message);
		virtual void	DispatchMessage(BMessage* message, BHandler* handler);

	private:
		void			_SendMessage();

		BTextControl	*fSearchString;
		BCheckBox		*fCaseSensBox;
		BCheckBox		*fWrapBox;
		BCheckBox		*fBackSearchBox;
		BButton			*fCancelButton;
		BButton			*fSearchButton;

		BHandler		*fHandler;
};

#endif	// FIND_WINDOW_H

