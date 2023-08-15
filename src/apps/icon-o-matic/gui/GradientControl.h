/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef GRADIENT_CONTROL_H
#define GRADIENT_CONTROL_H


#include <View.h>

#if LIB_LAYOUT
#	include <layout.h>
#endif

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE
	class Gradient;
_END_ICON_NAMESPACE


enum {
	MSG_GRADIENT_CONTROL_FOCUS_CHANGED	= 'gcfc',
};

class GradientControl :
						#if LIB_LAYOUT
						public MView,
						#endif
						public BView {

 public:
								GradientControl(BMessage* message = NULL,
												BHandler* target = NULL);
	virtual						~GradientControl();

								// MView
	#if LIB_LAYOUT
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);
	#endif

								// BView
	virtual	void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focus);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				GetPreferredSize(float* width, float* height);

								// GradientControl
			void				SetGradient(const _ICON_NAMESPACE Gradient*
											gradient);
			_ICON_NAMESPACE Gradient* Gradient() const
									{ return fGradient; }

			void				SetCurrentStop(const rgb_color& color);
			bool				GetCurrentStop(rgb_color* color) const;

			void				SetEnabled(bool enabled);
			bool				IsEnabled() const
									{ return fEnabled; }

 private:
			void				_UpdateColors();
			void				_AllocBitmap(int32 width, int32 height);
			BRect				_GradientBitmapRect() const;
			int32				_StepIndexFor(BPoint where) const;
			float				_OffsetFor(BPoint where) const;
			void				_UpdateCurrentColor() const;

 			_ICON_NAMESPACE Gradient* fGradient;
			BBitmap*			fGradientBitmap;
			int32				fDraggingStepIndex;
			int32				fCurrentStepIndex;

			float				fDropOffset;
			int32				fDropIndex;

			bool				fEnabled;

			BMessage*			fMessage;
			BHandler*			fTarget;
};

#endif // GRADIENT_CONTROL_H
