/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef ICON_OPTIONS_CONTROL_H
#define ICON_OPTIONS_CONTROL_H

#include <Control.h>
#include <Invoker.h>

#include <MDividable.h>

class IconButton;

class IconOptionsControl : public MView,
						   public MDividable,
						   public BControl {
 public:
								IconOptionsControl(const char* name = NULL,
												   const char* label = NULL,
												   BMessage* message = NULL,
												   BHandler* target = NULL);
								~IconOptionsControl();

	// MView interface
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);

	// MDividable interface
	virtual float				LabelWidth();

	// BControl interface
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				SetLabel(const char* label);
	virtual	void				SetValue(int32 value);
	virtual	int32				Value() const;
	virtual	void				SetEnabled(bool enable);

	virtual	void				MessageReceived(BMessage* message);

	// BInvoker interface
	virtual	status_t			Invoke(BMessage* message = NULL);

	// IconOptionsControl
			void				AddOption(IconButton* icon);

 private:

			IconButton*			_FindIcon(int32 index) const;
			void				_TriggerRelayout();
			void				_LayoutIcons(BRect frame);

			BHandler*			fTargetCache;
};

#endif // ICON_OPTIONS_CONTROL_H
