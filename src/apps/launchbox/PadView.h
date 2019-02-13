/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PAD_VIEW_H
#define PAD_VIEW_H

#include <View.h>

class BGroupLayout;
class LaunchButton;

#define DEFAULT_ICON_SIZE 32

class PadView : public BView {
public:
								PadView(const char* name);
	virtual						~PadView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	// PadView
			void				AddButton(LaunchButton* button,
									LaunchButton* beforeButton = NULL);
			bool				RemoveButton(LaunchButton* button);
			LaunchButton*		ButtonAt(int32 index) const;

			void				DisplayMenu(BPoint where,
									LaunchButton* button = NULL) const;

			void				SetOrientation(enum orientation orientation);
			enum orientation	Orientation() const;

			void				SetIconSize(uint32 size);
			uint32				IconSize() const;

			void				SetIgnoreDoubleClick(bool refuse);
			bool				IgnoreDoubleClick() const;

private:
			void				_NotifySettingsChanged();

			BPoint				fDragOffset;
			bool				fDragging;
			bigtime_t			fClickTime;
			BGroupLayout*		fButtonLayout;
			uint32				fIconSize;
};

#endif // PAD_VIEW_H