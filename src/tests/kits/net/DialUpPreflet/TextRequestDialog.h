//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _TEXT_REQUEST_DIALOG__H
#define _TEXT_REQUEST_DIALOG__H

#include <Window.h>


class TextRequestDialog : public BWindow {
	public:
		TextRequestDialog(const char *title, const char *request,
			const char *text = NULL);
		virtual ~TextRequestDialog();
		
		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
		
		status_t Go(BInvoker *invoker);

	private:
		BTextControl *fTextControl;
		BInvoker *fInvoker;
};


#endif
