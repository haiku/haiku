/*
 * Copyright 2006-2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef NAME_PANEL_H
#define NAME_PANEL_H

#include "Panel.h"

class BTextControl;

class NamePanel : public Panel {
public:
							NamePanel(const char* label, const char* text,
								BWindow* window, BHandler* target,
								BMessage* message, const BSize& size);
	virtual					~NamePanel();

	virtual void			MessageReceived(BMessage *message);

private:
	BTextControl*			fNameTC;
	BWindow*				fWindow;
	BHandler*				fTarget;
	BMessage*				fMessage;

	window_feel				fSavedTargetWindowFeel;
};

#endif // NAME_PANEL_H
