/*
 * Copyright 2006-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef STYLE_VIEW_H
#define STYLE_VIEW_H


#include "IconBuild.h"
#include "Observer.h"

#include <View.h>


class BMenu;
class BMenuField;
class CommandStack;
class CurrentColor;
class GradientControl;

_BEGIN_ICON_NAMESPACE
	class Gradient;
	class Style;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE


// TODO: write lock the document when changing something...

enum {
	MSG_STYLE_TYPE_CHANGED	= 'stch',
};

class StyleView : public BView, public Observer {
public:
								StyleView(BRect frame);
	virtual						~StyleView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* message);

	virtual	BSize				MinSize();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// StyleView
			void				SetStyle(Style* style);
			void				SetCommandStack(CommandStack* stack);
			void				SetCurrentColor(CurrentColor* color);

private:
			void				_SetGradient(Gradient* gradient,
									bool forceControlUpdate = false,
									bool sendMessage = false);
			void				_MarkType(BMenu* menu, int32 type) const;
			void				_SetStyleType(int32 type);
			void				_SetGradientType(int32 type);
			void				_AdoptCurrentColor(rgb_color color);
			void				_TransferGradientStopColor();

private:
			CommandStack*		fCommandStack;
			CurrentColor*		fCurrentColor;

			Style*				fStyle;
			Gradient*			fGradient;
			bool				fIgnoreCurrentColorNotifications;
			bool				fIgnoreControlGradientNotifications;

			GradientControl*	fGradientControl;
			BMenuField*			fStyleType;
			BMenuField*			fGradientType;

			BRect				fPreviousBounds;
};


#endif // STYLE_VIEW_H
