/*
 * Copyright 2009-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TOOL_TIP_WINDOW_H
#define TOOL_TIP_WINDOW_H


#include <Window.h>


namespace BPrivate {


class ToolTipWindow : public BWindow {
public:
							ToolTipWindow(BToolTip* tip, BPoint where,
								void* owner);

	virtual	void			MessageReceived(BMessage* message);

private:
			void*			fOwner;
};


}	// namespace BPrivate


#endif	// TOOL_TIP_WINDOW_H
