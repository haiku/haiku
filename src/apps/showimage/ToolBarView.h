/*
 * Copyright 2011 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TOOL_BAR_VIEW_H
#define TOOL_BAR_VIEW_H

#include <GroupView.h>


namespace BPrivate {
class BIconButton;
}

using BPrivate::BIconButton;


class ToolBarView : public BGroupView {
public:
								ToolBarView(BRect frame);
	virtual						~ToolBarView();

			void				AddAction(uint32 command, BHandler* target,
									const BBitmap* icon,
									const char* toolTipText = NULL);
			void				AddAction(BMessage* message, BHandler* target,
									const BBitmap* icon,
									const char* toolTipText = NULL);
			void				AddSeparator();

			void				SetActionEnabled(uint32 command, bool enabled);
			void				SetActionPressed(uint32 command, bool pressed);

private:
			void				_AddView(BView* view);
			BIconButton*		_FindIconButton(uint32 command) const;
};

#endif // TOOL_BAR_VIEW_H
