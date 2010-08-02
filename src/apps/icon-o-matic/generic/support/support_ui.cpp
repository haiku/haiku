/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "support_ui.h"

#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <DataIO.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Screen.h>
#include <String.h>
#include <Path.h>
#include <View.h>

// stroke_frame
void
stroke_frame(BView* v, BRect r, rgb_color left, rgb_color top,
			 rgb_color right, rgb_color bottom)
{
	if (v && r.IsValid()) {
		v->BeginLineArray(4);
			v->AddLine(BPoint(r.left, r.bottom),
					   BPoint(r.left, r.top), left);
			v->AddLine(BPoint(r.left + 1.0, r.top),
					   BPoint(r.right, r.top), top);
			v->AddLine(BPoint(r.right, r.top + 1.0),
					   BPoint(r.right, r.bottom), right);
			v->AddLine(BPoint(r.right - 1.0, r.bottom),
					   BPoint(r.left + 1.0, r.bottom), bottom);
		v->EndLineArray();
	}
}

// store_color_in_message
status_t
store_color_in_message(BMessage* message, rgb_color color)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
		ret = message->AddData("RGBColor", B_RGB_COLOR_TYPE,
							   (void*)&color, sizeof(rgb_color));
	}
	return ret;
}

// restore_color_from_message
status_t
restore_color_from_message(const BMessage* message, rgb_color& color, int32 index)
{
	status_t ret = B_BAD_VALUE;
	if (message) {
			const void* colorPointer;
			ssize_t size = sizeof(rgb_color);
			ret = message->FindData("RGBColor", B_RGB_COLOR_TYPE, index,
									&colorPointer, &size);
			if (ret >= B_OK)
				color = *(const rgb_color*)colorPointer;
	}
	return ret;
}

// make_color_drop_message
BMessage
make_color_drop_message(rgb_color color, BBitmap* bitmap)
{
	// prepare message
	BMessage message(B_PASTE);
	char hexString[8];
	snprintf(hexString, sizeof(hexString), "#%.2X%.2X%.2X",
		color.red, color.green, color.blue);
	message.AddData("text/plain", B_MIME_TYPE, &hexString, sizeof(hexString));
	message.AddData("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof(color));
	// prepare bitmap
	if (bitmap && bitmap->IsValid()
		&& (bitmap->ColorSpace() == B_RGB32
			|| bitmap->ColorSpace() == B_RGBA32)) {
		uint8* bits = (uint8*)bitmap->Bits();
		uint32 bpr = bitmap->BytesPerRow();
		uint32 width = bitmap->Bounds().IntegerWidth() + 1;
		uint32 height = bitmap->Bounds().IntegerHeight() + 1;
		for (uint32 y = 0; y < height; y++) {
			uint8* bitsHandle = bits;
			for (uint32 x = 0; x < width; x++) {
				if (x == 0 || y == 0 ) {
					// top or left border
					bitsHandle[0] = (uint8)min_c(255, color.blue * 1.2 + 40);
					bitsHandle[1] = (uint8)min_c(255, color.green * 1.2 + 40);
					bitsHandle[2] = (uint8)min_c(255, color.red * 1.2 + 40);
					bitsHandle[3] = 180;
				} else if ((x == width - 2 || y == height - 2)
						   && !(x == width - 1 || y == height - 1)) {
					// bottom or right border
					bitsHandle[0] = (uint8)(color.blue * 0.8);
					bitsHandle[1] = (uint8)(color.green * 0.8);
					bitsHandle[2] = (uint8)(color.red * 0.8);
					bitsHandle[3] = 180;
				} else if (x == width - 1 || y == height - 1) {
					// shadow
					bitsHandle[0] = 0;
					bitsHandle[1] = 0;
					bitsHandle[2] = 0;
					bitsHandle[3] = 100;
				} else {
					// color
					bitsHandle[0] = color.blue;
					bitsHandle[1] = color.green;
					bitsHandle[2] = color.red;
					bitsHandle[3] = 180;
				}
				if ((x == 0 && y == height - 1) || (y == 0 && x == width - 1)) {
					// spare pixels of shadow
					bitsHandle[0] = 0;
					bitsHandle[1] = 0;
					bitsHandle[2] = 0;
					bitsHandle[3] = 50;
				}
				bitsHandle += 4;
			}
			bits += bpr;
		}
	}
	return message;
}

// make_sure_frame_is_on_screen
void
make_sure_frame_is_on_screen(BRect& frame, BWindow* window)
{
	if (!frame.IsValid())
		return;

	BRect screenFrame;
	if (window) {
		BScreen screen(window);
		if (!screen.IsValid())
			return;
		screenFrame = screen.Frame();
	} else {
		BScreen screen(B_MAIN_SCREEN_ID);
		if (!screen.IsValid())
			return;
		screenFrame = screen.Frame();
	}
	if (!screenFrame.Contains(frame)) {
		// make sure frame fits in the screen
		if (frame.Width() > screenFrame.Width())
			frame.right -= frame.Width() - screenFrame.Width() + 10.0;
		if (frame.Height() > screenFrame.Height())
			frame.bottom -= frame.Height() - screenFrame.Height() + 30.0;
		// frame is now at the most the size of the screen
		if (frame.right > screenFrame.right)
			frame.OffsetBy(-(frame.right - screenFrame.right), 0.0);
		if (frame.bottom > screenFrame.bottom)
			frame.OffsetBy(0.0, -(frame.bottom - screenFrame.bottom));
		if (frame.left < screenFrame.left)
			frame.OffsetBy((screenFrame.left - frame.left), 0.0);
		if (frame.top < screenFrame.top)
			frame.OffsetBy(0.0, (screenFrame.top - frame.top));
	}
}

// print_modifiers
void
print_modifiers()
{
	uint32 mods = modifiers();
	if (mods & B_SHIFT_KEY)
		printf("B_SHIFT_KEY\n");
	if (mods & B_COMMAND_KEY)
		printf("B_COMMAND_KEY\n");
	if (mods & B_CONTROL_KEY)
		printf("B_CONTROL_KEY\n");
	if (mods & B_CAPS_LOCK)
		printf("B_CAPS_LOCK\n");
	if (mods & B_SCROLL_LOCK)
		printf("B_SCROLL_LOCK\n");
	if (mods & B_NUM_LOCK)
		printf("B_NUM_LOCK\n");
	if (mods & B_OPTION_KEY)
		printf("B_OPTION_KEY\n");
	if (mods & B_MENU_KEY)
		printf("B_MENU_KEY\n");
	if (mods & B_LEFT_SHIFT_KEY)
		printf("B_LEFT_SHIFT_KEY\n");
	if (mods & B_RIGHT_SHIFT_KEY)
		printf("B_RIGHT_SHIFT_KEY\n");
	if (mods & B_LEFT_COMMAND_KEY)
		printf("B_LEFT_COMMAND_KEY\n");
	if (mods & B_RIGHT_COMMAND_KEY)
		printf("B_RIGHT_COMMAND_KEY\n");
	if (mods & B_LEFT_CONTROL_KEY)
		printf("B_LEFT_CONTROL_KEY\n");
	if (mods & B_RIGHT_CONTROL_KEY)
		printf("B_RIGHT_CONTROL_KEY\n");
	if (mods & B_LEFT_OPTION_KEY)
		printf("B_LEFT_OPTION_KEY\n");
	if (mods & B_RIGHT_OPTION_KEY)
		printf("B_RIGHT_OPTION_KEY\n");
}

/*
// convert_cap_mode
agg::line_cap_e
convert_cap_mode(uint32 mode)
{
	agg::line_cap_e aggMode = agg::butt_cap;
	switch (mode) {
		case CAP_MODE_BUTT:
			aggMode = agg::butt_cap;
			break;
		case CAP_MODE_SQUARE:
			aggMode = agg::square_cap;
			break;
		case CAP_MODE_ROUND:
			aggMode = agg::round_cap;
			break;
	}
	return aggMode;
}

// convert_cap_mode
agg::line_join_e
convert_join_mode(uint32 mode)
{
	agg::line_join_e aggMode = agg::miter_join;
	switch (mode) {
		case JOIN_MODE_MITER:
			aggMode = agg::miter_join;
			break;
		case JOIN_MODE_ROUND:
			aggMode = agg::round_join;
			break;
		case JOIN_MODE_BEVEL:
			aggMode = agg::bevel_join;
			break;
	}
	return aggMode;
}
*/

// string_for_color_space
const char*
string_for_color_space(color_space format)
{
	const char* name = "<unkown format>";
	switch (format) {
		case B_RGB32:
			name = "B_RGB32";
			break;
		case B_RGBA32:
			name = "B_RGBA32";
			break;
		case B_RGB32_BIG:
			name = "B_RGB32_BIG";
			break;
		case B_RGBA32_BIG:
			name = "B_RGBA32_BIG";
			break;
		case B_RGB24:
			name = "B_RGB24";
			break;
		case B_RGB24_BIG:
			name = "B_RGB24_BIG";
			break;
		case B_CMAP8:
			name = "B_CMAP8";
			break;
		case B_GRAY8:
			name = "B_GRAY8";
			break;
		case B_GRAY1:
			name = "B_GRAY1";
			break;

		// YCbCr
		case B_YCbCr422:
			name = "B_YCbCr422";
			break;
		case B_YCbCr411:
			name = "B_YCbCr411";
			break;
		case B_YCbCr444:
			name = "B_YCbCr444";
			break;
		case B_YCbCr420:
			name = "B_YCbCr420";
			break;

		// YUV
		case B_YUV422:
			name = "B_YUV422";
			break;
		case B_YUV411:
			name = "B_YUV411";
			break;
		case B_YUV444:
			name = "B_YUV444";
			break;
		case B_YUV420:
			name = "B_YUV420";
			break;

		case B_YUV9:
			name = "B_YUV9";
			break;
		case B_YUV12:
			name = "B_YUV12";
			break;

		default:
			break;
	}
	return name;
}

// print_color_space
void
print_color_space(color_space format)
{
	printf("%s\n", string_for_color_space(format));
}

