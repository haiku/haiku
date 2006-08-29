/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STYLE_VIEW_H
#define STYLE_VIEW_H

#include <View.h>

#include "Observer.h"

class BMenu;
class BMenuField;
class CommandStack;
class CurrentColor;
class Gradient;
class GradientControl;
class Style;

// TODO: write lock the document when changing something...

enum {
	MSG_GRADIENT_SELECTED	= 'grsl',
};

class StyleView : public BView,
				  public Observer {
 public:
								StyleView(BRect frame);
	virtual						~StyleView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

#if __HAIKU__
	virtual	BSize				MinSize();
#endif

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// StyleView
			void				SetStyle(Style* style);
			void				SetCommandStack(CommandStack* stack);
			void				SetCurrentColor(CurrentColor* color);

 private:
			void				_SetGradient(Gradient* gradient);
			void				_MarkType(BMenu* menu,
										  int32 type) const;
			void				_SetStyleType(int32 type);
			void				_SetGradientType(int32 type);
			void				_AdoptCurrentColor(rgb_color color);
			void				_TransferGradientStopColor();


			CommandStack*		fCommandStack;
			CurrentColor*		fCurrentColor;

			Style*				fStyle;
			Gradient*			fGradient;
			bool				fIgnoreCurrentColorNotifications;

			GradientControl*	fGradientControl;
			BMenuField*			fStyleType;
			BMenuField*			fGradientType;
};

#endif // STYLE_VIEW_H
