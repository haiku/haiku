/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */
#ifndef HYPER_TEXT_ACTIONS_H
#define HYPER_TEXT_ACTIONS_H

#include <String.h>

#include "HyperTextView.h"


class URLAction : public HyperTextAction {
public:
								URLAction(const BString& url);
	virtual						~URLAction();

	virtual	void				Clicked(HyperTextView* view, BPoint where,
									BMessage* message);
private:
			BString				fURL;
};


class OpenFileAction : public HyperTextAction {
public:
								OpenFileAction(const BString& file);
	virtual						~OpenFileAction();

	virtual	void				Clicked(HyperTextView* view, BPoint where,
									BMessage* message);
private:
			BString				fFile;
};


#endif	// HYPER_TEXT_ACTIONS_H
