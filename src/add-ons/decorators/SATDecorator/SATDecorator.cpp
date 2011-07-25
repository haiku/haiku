/*
 * Copyright 2010, Haiku, Inc.
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


static const float kResizeKnobSize = 18.0;

static const rgb_color kHighlightFrameColors[6] = {
	{ 52, 52, 52, 255 },
	{ 140, 140, 140, 255 },
	{ 124, 124, 124, 255 },
	{ 108, 108, 108, 255 },
	{ 52, 52, 52, 255 },
	{ 8, 8, 8, 255 }
};

static const rgb_color kTabColor = {255, 203, 0, 255};
static const rgb_color kHighlightTabColor = tint_color(kTabColor,
	B_DARKEN_2_TINT);
static const rgb_color kHighlightTabColorLight = tint_color(kHighlightTabColor,
	(B_LIGHTEN_MAX_TINT + B_LIGHTEN_2_TINT) / 2);
static const rgb_color kHighlightTabColorBevel = tint_color(kHighlightTabColor,
	B_LIGHTEN_2_TINT);
static const rgb_color kHighlightTabColorShadow = tint_color(kHighlightTabColor,
	(B_DARKEN_1_TINT + B_NO_TINT) / 2);


SATDecorAddOn::SATDecorAddOn(image_id id, const char* name)
	:
	DecorAddOn(id, name)
{
	fDesktopListeners.AddItem(&fStackAndTile);
}


status_t
SATDecorAddOn::InitCheck() const
{
	if (fDesktopListeners.CountItems() != 1)
		return B_ERROR;

	return B_OK;
}


WindowBehaviour*
SATDecorAddOn::AllocateWindowBehaviour(Window* window)
{
	return new (std::nothrow)SATWindowBehaviour(window, &fStackAndTile);
}


Decorator*
SATDecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	window_look look, uint32 flags)
{
	return new (std::nothrow)SATDecorator(settings, rect, look, flags);
}


SATDecorator::SATDecorator(DesktopSettings& settings, BRect frame,
	window_look look, uint32 flags)
	:
	DefaultDecorator(settings, frame, look, flags)
{

}


void
SATDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	DefaultDecorator::Tab* tab = static_cast<DefaultDecorator::Tab*>(_tab);
	// we handle only our own highlights
	if (highlight != HIGHLIGHT_STACK_AND_TILE) {
		DefaultDecorator::GetComponentColors(component, highlight,
			_colors, tab);
		return;
	}

	if (tab && tab->isHighlighted == false
		&& (component == COMPONENT_TAB || component == COMPONENT_CLOSE_BUTTON
			|| component == COMPONENT_ZOOM_BUTTON)) {
		DefaultDecorator::GetComponentColors(component, highlight,
			_colors, tab);
		return;
	}

	switch (component) {
		case COMPONENT_TAB:
			_colors[COLOR_TAB_FRAME_LIGHT] = kFrameColors[0];
			_colors[COLOR_TAB_FRAME_DARK] = kFrameColors[3];
			_colors[COLOR_TAB] = kHighlightTabColor;
			_colors[COLOR_TAB_LIGHT] = kHighlightTabColorLight;
			_colors[COLOR_TAB_BEVEL] = kHighlightTabColorBevel;
			_colors[COLOR_TAB_SHADOW] = kHighlightTabColorShadow;
			_colors[COLOR_TAB_TEXT] = kFocusTextColor;
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			_colors[COLOR_BUTTON] = kHighlightTabColor;
			_colors[COLOR_BUTTON_LIGHT] = kHighlightTabColorLight;
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


extern "C" DecorAddOn* (instantiate_decor_addon)(image_id id, const char* name)
{
	return new (std::nothrow)SATDecorAddOn(id, name);
}
