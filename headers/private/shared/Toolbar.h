/*
 * Copyright 2011 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _TOOLBAR_H
#define _TOOLBAR_H

#include <GroupView.h>


class BButton;


class BToolbar : public BGroupView {
public:
								BToolbar(BRect frame);
	virtual						~BToolbar();

	virtual	void				Hide();

			void				AddAction(uint32 command, BHandler* target,
									const BBitmap* icon,
									const char* toolTipText = NULL,
									bool lockable = false);
			void				AddAction(BMessage* message, BHandler* target,
									const BBitmap* icon,
									const char* toolTipText = NULL,
									bool lockable = false);
			void				AddSeparator();
			void				AddGlue();

			void				SetActionEnabled(uint32 command, bool enabled);
			void				SetActionPressed(uint32 command, bool pressed);
			void				SetActionVisible(uint32 command, bool visible);

private:
	virtual	void				Pulse();
	virtual	void				FrameResized(float width, float height);

			void				_AddView(BView* view);
			BButton*			_FindButton(uint32 command) const;
			void				_HideToolTips() const;
};

#endif // _TOOLBAR_H
