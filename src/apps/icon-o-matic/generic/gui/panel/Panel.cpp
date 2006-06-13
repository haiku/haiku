/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Panel.h"

#include <stdio.h>

#include <InterfaceDefs.h>
#include <Message.h>
#include <MessageFilter.h>

class EscapeFilter : public BMessageFilter {
 public:
	EscapeFilter(Panel* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fPanel(target)
		{
		}
	virtual	~EscapeFilter()
		{
		}
	virtual	filter_result	Filter(BMessage* message, BHandler** target)
		{
			filter_result result = B_DISPATCH_MESSAGE;
			switch (message->what) {
				case B_KEY_DOWN:
				case B_UNMAPPED_KEY_DOWN: {
					uint32 key;
					if (message->FindInt32("raw_char", (int32*)&key) >= B_OK) {
						if (key == B_ESCAPE) {
							result = B_SKIP_MESSAGE;
							fPanel->Cancel();
						}
					}
					break;
				}
				default:
					break;
			}
			return result;
		}
 private:
 	Panel*		fPanel;
};

// constructor
Panel::Panel(BRect frame, const char* title,
			 window_type type, uint32 flags,
			 uint32 workspace)
	: BWindow(frame, title, type, flags, workspace)
{
	_InstallFilter();
}

// constructor
Panel::Panel(BRect frame, const char* title,
			 window_look look, window_feel feel,
			 uint32 flags, uint32 workspace)
	: BWindow(frame, title, look, feel, flags, workspace)
{
	_InstallFilter();
}

// destructor
Panel::~Panel()
{
}

// MessageReceived
void
Panel::Cancel()
{
	PostMessage(B_QUIT_REQUESTED);
}

// _InstallFilter
void
Panel::_InstallFilter()
{
	AddCommonFilter(new EscapeFilter(this));
}
