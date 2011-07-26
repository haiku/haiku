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


class SATDecorator : public DefaultDecorator {
public:
			enum {
				HIGHLIGHT_STACK_AND_TILE = HIGHLIGHT_USER_DEFINED
			};

public:
								SATDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);

protected:
	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors,
									Decorator::Tab* tab = NULL);
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
