/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef REPLACE_WINDOW_H 
#define REPLACE_WINDOW_H 


#include <Window.h>
#include <Rect.h>
#include <Handler.h>
#include <String.h>
#include <Message.h>
#include <View.h>
#include <TextControl.h>
#include <CheckBox.h>
#include <Button.h>


class ReplaceWindow : public BWindow {
	public:
		ReplaceWindow(BRect frame, BHandler *_handler,BString *searchString,
			BString *replaceString, bool *caseState, bool *wrapState, bool *backState);

		virtual void MessageReceived(BMessage* message);
		virtual void DispatchMessage(BMessage* message, BHandler *handler);

	private:
		void _SendMessage(uint32 what);
		void _ChangeUI();		

		BView 			*fReplaceView;
		BTextControl	*fSearchString;
		BTextControl	*fReplaceString;
		BCheckBox		*fCaseSensBox;
		BCheckBox		*fWrapBox;
		BCheckBox		*fBackSearchBox;
		BCheckBox		*fAllWindowsBox;
		BButton			*fReplaceButton;
		BButton			*fReplaceAllButton;
		BButton			*fCancelButton;
		BHandler		*fHandler;
		bool			fUIchange;
};

#endif	// REPLACE_WINDOW_H
