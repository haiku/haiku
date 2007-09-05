/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mattias Sundblad
 *		Andrew Bachmann
 */
#ifndef STYLED_EDIT_APP
#define STYLED_EDIT_APP


#include <Application.h>


struct entry_ref;

class BMenu;
class BHandler;
class BMessage;
class BFilePanel;
class StyledEditWindow;


class StyledEditApp : public BApplication {
	public:
						StyledEditApp();
		virtual			~StyledEditApp();

		virtual void	MessageReceived(BMessage *message);
		virtual void	RefsReceived(BMessage *message);
		virtual void	ReadyToRun();

		virtual void	DispatchMessage(BMessage *an_event, BHandler *handler);

		int32			NumberOfWindows();
		void			OpenDocument();
		void			OpenDocument(entry_ref *ref);
		void			CloseDocument();

	private:
		void			ArgvReceivedEx(int32 argc, const char *argv[], const char * cwd);

	private:
		BFilePanel		*fOpenPanel;
		BMenu			*fOpenPanelEncodingMenu;
		uint32			fOpenAsEncoding;
		int32			fWindowCount;
		int32			fNextUntitledWindow;
		
};

extern StyledEditApp* styled_edit_app;

#endif	// STYLED_EDIT_APP

