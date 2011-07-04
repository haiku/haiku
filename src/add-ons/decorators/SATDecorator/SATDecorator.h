/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_DECORATOR_H
#define SAT_DECORATOR_H


#include "DecorManager.h"
#include "DefaultDecorator.h"
#include "DefaultWindowBehaviour.h"
#include "StackAndTile.h"


class SATDecorAddOn : public DecorAddOn {
public:
								SATDecorAddOn(image_id id, const char* name);

	virtual status_t			InitCheck() const;

	virtual	WindowBehaviour*	AllocateWindowBehaviour(Window* window);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);

			StackAndTile		fStackAndTile;
};


class SATDecorator : public DefaultDecorator {
public:
			enum {
				HIGHLIGHT_STACK_AND_TILE = HIGHLIGHT_USER_DEFINED
			};

public:
								SATDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);

			/*! Indicates that window is stacked */
			void				SetStackedMode(bool stacked, BRegion* dirty);
			bool				StackedMode() const
									{ return fStackedMode; }

			/*! Set the tab length if the decorator is in stacked mode and if
				the tab is the last one in the tab bar. */
			void				SetStackedTabLength(float length,
									BRegion* dirty);
			float				StackedTabLength() const
									{ return fStackedTabLength; }

protected:
			void				_DoLayout();
			void				_LayoutTabItems(const BRect& tabRect);

			bool				_SetTabLocation(float location,
									BRegion* updateRegion = NULL);
			void				_SetFocus();

	virtual	void				DrawButtons(const BRect& invalid);
	virtual	void				GetComponentColors(Component component,
									 uint8 highlight, ComponentColors _colors);

private:
			bool				fStackedMode;
			bool				fStackedDrawZoom;
			float				fStackedTabLength;
			bool				fStackedTabShifting;
};


class SATWindowBehaviour : public DefaultWindowBehaviour {
public:
								SATWindowBehaviour(Window* window,
									StackAndTile* sat);

protected:
	virtual bool				AlterDeltaForSnap(Window* window, BPoint& delta,
									bigtime_t now);

private:
			StackAndTile*		fStackAndTile;
};


#endif
