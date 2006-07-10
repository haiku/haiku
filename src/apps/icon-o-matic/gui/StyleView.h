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

class BMenuField;
class CommandStack;
class Gradient;
class GradientControl;
class Style;

// TODO: write lock the document when changing something...

class StyleView : public BView,
				  public Observer {
 public:
								StyleView(BRect frame);
	virtual						~StyleView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// StyleView
			void				SetStyle(Style* style);
			void				SetCommandStack(CommandStack* stack);

 private:
			void				_SetGradient(Gradient* gradient);
			void				_MarkType(BMenu* menu,
										  int32 type) const;
			void				_SetStyleType(int32 type);
			void				_SetGradientType(int32 type);


			CommandStack*		fCommandStack;

			Style*				fStyle;
			Gradient*			fGradient;

			GradientControl*	fGradientControl;
			BMenuField*			fStyleType;
			BMenuField*			fGradientType;
};

#endif // STYLE_VIEW_H
