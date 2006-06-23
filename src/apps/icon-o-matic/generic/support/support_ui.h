/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SUPPORT_UI_H
#define SUPPORT_UI_H

#include <GraphicsDefs.h>
#include <Rect.h>
#include <agg_math_stroke.h>

class BBitmap;
class BDataIO;
class BMessage;
class BPositionIO;
class BString;
class BView;
class BWindow;

// looper of view must be locked!
void stroke_frame(BView* view, BRect frame,
				  rgb_color left, rgb_color top,
				  rgb_color right, rgb_color bottom);


status_t store_color_in_message(BMessage* message, rgb_color color);

status_t restore_color_from_message(const BMessage* message, rgb_color& color, int32 index = 0);

BMessage make_color_drop_message(rgb_color color, BBitmap* bitmap);

void make_sure_frame_is_on_screen(BRect& frame, BWindow* window);

void print_modifiers();

//agg::line_cap_e convert_cap_mode(uint32 mode);
//agg::line_join_e convert_join_mode(uint32 mode);

const char* string_for_color_space(color_space format);
void print_color_space(color_space format);


// Those are already defined in newer versions of BeOS
#if !defined(B_BEOS_VERSION_DANO) && !defined(__HAIKU__)

// rgb_color == rgb_color
static inline bool
operator==(const rgb_color& a, const rgb_color& b)
{
	return a.red == b.red
			&& a.green == b.green
			&& a.blue == b.blue
			&& a.alpha == b.alpha;
}

// rgb_color != rgb_color
static inline bool
operator!=(const rgb_color& a, const rgb_color& b)
{
	return !(a == b);
}

#endif // B_BEOS_VERSION <= ...

#endif // SUPPORT_UI_H
