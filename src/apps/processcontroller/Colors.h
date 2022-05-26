/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

//Useful until be gets around to making these sorts of things
//globals akin to be_plain_font, etc.

#ifndef _COLORS_H_
#define _COLORS_H_

#include <GraphicsDefs.h>

//	ui_color(B_PANEL_BACKGROUND_COLOR)
//	ui_color(B_MENU_BACKGROUND_COLOR)
//	ui_color(B_WINDOW_TAB_COLOR)
//	ui_color(B_KEYBOARD_NAVIGATION_COLOR)
//	ui_color(B_DESKTOP_COLOR)
//	tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT)

//Other colors
const rgb_color kBlack =					{0,  0,  0,		255};
const rgb_color kWhite =					{255,255,255,	255};
const rgb_color kRed =						{255,0,  0,		255};
const rgb_color kGreen =					{0,  203,0,		255};
const rgb_color kLightGreen =				{90, 240,90,	255};
const rgb_color kBlue =						{49, 61, 225,	255};
const rgb_color kLightBlue =				{64, 162,255,	255};
const rgb_color kPurple =					{144,64, 221,	255};
const rgb_color kLightPurple =				{166,74, 255,	255};
const rgb_color kLavender =					{193,122,255,	255};
const rgb_color kYellow =					{255,203,0,		255};
const rgb_color kOrange =					{255,163,0,		255};
const rgb_color kFlesh =					{255,231,186,	255};
const rgb_color kTan =						{208,182,121,	255};
const rgb_color kBrown =					{154,110,45,	255};
const rgb_color kLightMetallicBlue =		{143,166,240,	255};
const rgb_color kMedMetallicBlue =			{75, 96, 154,	255};
const rgb_color kDarkMetallicBlue =			{78, 89, 126,	255};

const rgb_color kGebHighlight =				{152, 152, 203,	255};
const rgb_color kBordeaux =					{80, 0, 0,		255};

#endif // _COLORS_H_
