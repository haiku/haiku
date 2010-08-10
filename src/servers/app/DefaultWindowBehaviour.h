/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef DEFAULT_WINDOW_BEHAVIOUR_H
#define DEFAULT_WINDOW_BEHAVIOUR_H


#include "WindowBehaviour.h"

#include "Decorator.h"


class Desktop;
class Window;


class DefaultWindowBehaviour : public WindowBehaviour {
public:
								DefaultWindowBehaviour(Window* window);
	virtual						~DefaultWindowBehaviour();

	virtual	bool				MouseDown(BMessage* message, BPoint where);
	virtual	void				MouseUp(BMessage* message, BPoint where);
	virtual	void				MouseMoved(BMessage *message, BPoint where,
									bool isFake);
		
private:
			bool				_IsWindowModifier(int32 modifiers) const;
			click_type			_ActionFor(const BMessage* message,
									int32 buttons, int32 modifiers) const;
			void				_AlterDeltaForSnap(BPoint& delta,
									bigtime_t now);

protected:
			Window*				fWindow;
			Desktop*			fDesktop;
	
			bool				fIsClosing : 1;
			bool				fIsMinimizing : 1;
			bool				fIsZooming : 1;
			bool				fIsSlidingTab : 1;
			bool				fActivateOnMouseUp : 1;
	
			BPoint				fLastMousePosition;
			float				fMouseMoveDistance;
			bigtime_t			fLastMoveTime;
			bigtime_t			fLastSnapTime;
};


#endif	// DEFAULT_WINDOW_BEHAVIOUR_H
