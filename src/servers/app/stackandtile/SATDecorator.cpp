/*
 * Copyright 2010-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "SATDecorator.h"

#include <new>

#include <GradientLinear.h>
#include <WindowPrivate.h>

#include "DrawingEngine.h"
#include "SATWindow.h"


//#define DEBUG_SATDECORATOR
#ifdef DEBUG_SATDECORATOR
#	define STRACE(x) debug_printf x
#else
#	define STRACE(x) ;
#endif


static const rgb_color kFrameColors[4] = {
	{ 152, 152, 152, 255 },
	{ 240, 240, 240, 255 },
	{ 152, 152, 152, 255 },
	{ 108, 108, 108, 255 }
};

static const rgb_color kHighlightFrameColors[6] = {
	{ 52, 52, 52, 255 },
	{ 140, 140, 140, 255 },
	{ 124, 124, 124, 255 },
	{ 108, 108, 108, 255 },
	{ 52, 52, 52, 255 },
	{ 8, 8, 8, 255 }
};

SATDecorator::SATDecorator(DesktopSettings& settings, BRect frame,
							Desktop* desktop)
	:
	DefaultDecorator(settings, frame, desktop)
{
}


void
SATDecorator::UpdateColors(DesktopSettings& settings)
{
	DefaultDecorator::UpdateColors(settings);

	// Called during construction, and during any changes
	fHighlightTabColor		= tint_color(fFocusTabColor, B_DARKEN_2_TINT);
	fHighlightTabColorLight	= tint_color(fHighlightTabColor,
								(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2);
	fHighlightTabColorBevel	= tint_color(fHighlightTabColor, B_LIGHTEN_2_TINT);
	fHighlightTabColorShadow= tint_color(fHighlightTabColor,
								(B_DARKEN_1_TINT + B_NO_TINT) / 2);
}


void
SATDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);

	// Get the standard colors from the DefaultDecorator
	DefaultDecorator::GetComponentColors(component, highlight, _colors, tab);

	// Now we need to make some changes if the Stack and tile highlight is used
	if (highlight != HIGHLIGHT_STACK_AND_TILE)
		return;

	if (tab && tab->isHighlighted == false
		&& (component == COMPONENT_TAB || component == COMPONENT_CLOSE_BUTTON
			|| component == COMPONENT_ZOOM_BUTTON)) {
		return;
	}

	switch (component) {
		case COMPONENT_TAB:
			_colors[COLOR_TAB_FRAME_LIGHT] = kFrameColors[0];
			_colors[COLOR_TAB_FRAME_DARK] = kFrameColors[3];
			_colors[COLOR_TAB] = fHighlightTabColor;
			_colors[COLOR_TAB_LIGHT] = fHighlightTabColorLight;
			_colors[COLOR_TAB_BEVEL] = fHighlightTabColorBevel;
			_colors[COLOR_TAB_SHADOW] = fHighlightTabColorShadow;
			_colors[COLOR_TAB_TEXT] = fFocusTextColor;
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			_colors[COLOR_BUTTON] = fHighlightTabColor;
			_colors[COLOR_BUTTON_LIGHT] = fHighlightTabColorLight;
			break;

		case COMPONENT_LEFT_BORDER:
		case COMPONENT_RIGHT_BORDER:
		case COMPONENT_TOP_BORDER:
		case COMPONENT_BOTTOM_BORDER:
		case COMPONENT_RESIZE_CORNER:
		default:
			_colors[0] = kHighlightFrameColors[0];
			_colors[1] = kHighlightFrameColors[1];
			_colors[2] = kHighlightFrameColors[2];
			_colors[3] = kHighlightFrameColors[3];
			_colors[4] = kHighlightFrameColors[4];
			_colors[5] = kHighlightFrameColors[5];
			break;
	}
}


SATWindowBehaviour::SATWindowBehaviour(Window* window, StackAndTile* sat)
	:
	DefaultWindowBehaviour(window),

	fStackAndTile(sat)
{
}


bool
SATWindowBehaviour::AlterDeltaForSnap(Window* window, BPoint& delta,
	bigtime_t now)
{
	if (DefaultWindowBehaviour::AlterDeltaForSnap(window, delta, now) == true)
		return true;

	SATWindow* satWindow = fStackAndTile->GetSATWindow(window);
	if (satWindow == NULL)
		return false;
	SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return false;

	BRect groupFrame = group->WindowAt(0)->CompleteWindowFrame();
	for (int32 i = 1; i < group->CountItems(); i++)
		groupFrame = groupFrame | group->WindowAt(i)->CompleteWindowFrame();

	return fMagneticBorder.AlterDeltaForSnap(window->Screen(),
		groupFrame, delta, now);
}
