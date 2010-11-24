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
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
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
			enum Region {
				REGION_NONE,

				REGION_TAB,
				REGION_BORDER,

				REGION_CLOSE_BUTTON,
				REGION_ZOOM_BUTTON,
				REGION_MINIMIZE_BUTTON,

				REGION_RESIZE_CORNER
			};

			enum Action {
				ACTION_NONE,
				ACTION_ZOOM,
				ACTION_CLOSE,
				ACTION_MINIMIZE,
				ACTION_TAB,
				ACTION_DRAG,
				ACTION_SLIDE_TAB,
				ACTION_RESIZE,
				ACTION_RESIZE_BORDER
			};

			struct State;
			struct MouseTrackingState;
			struct DragState;
			struct ResizeState;
			struct SlideTabState;
			struct ResizeBorderState;
			struct DecoratorButtonState;

			// to keep gcc 2 happy
			friend struct State;
			friend struct MouseTrackingState;
			friend struct DragState;
			friend struct ResizeState;
			friend struct SlideTabState;
			friend struct ResizeBorderState;
			friend struct DecoratorButtonState;

private:
			bool				_IsWindowModifier(int32 modifiers) const;
			Region				_RegionFor(const BMessage* message) const;

			void				_NextState(State* state);

protected:
			Window*				fWindow;
			Desktop*			fDesktop;
			State*				fState;
			int32				fLastModifiers;
			int32				fLastMouseButtons;
			Region				fLastRegion;
			int32				fResetClickCount;
};


#endif	// DEFAULT_WINDOW_BEHAVIOUR_H
