//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ColorSet.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for encapsulating GUI system colors
//					
//  
//------------------------------------------------------------------------------
#ifndef COLORSET_H_
#define COLORSET_H_

#include <Locker.h>
#include "RGBColor.h"
#include <Message.h>
#include <String.h>

/*!
	\class ColorSet ColorSet.h
	\brief Encapsulates GUI system colors
*/
class ColorSet : public BLocker
{
public:
	ColorSet(void);
	ColorSet(const ColorSet &cs);
	ColorSet & operator=(const ColorSet &cs);
	void SetColors(const ColorSet &cs);
	void PrintToStream(void) const;
	bool ConvertToMessage(BMessage *msg) const;
	bool ConvertFromMessage(const BMessage *msg);
	void SetToDefaults(void);
	RGBColor StringToColor(const char *string);
	status_t SetColor(const char *string, rgb_color value);
	
	BString name;
	
	RGBColor panel_background,
	panel_text,

	document_background,
	document_text,

	control_background,
	control_text,
	control_highlight,
	control_border,

	tooltip_background,
	tooltip_text,

	menu_background,
	menu_selected_background,
	menu_text,
	menu_selected_text,
	menu_selected_border,

	keyboard_navigation_base,
	keyboard_navigation_pulse,
	
	success,
	failure,
	shine,
	shadow,
	window_tab,

	// Not all of these guys don't exist in InterfaceDefs.h, but we keep 
	// them as part of the color set anyway - they're important nonetheless
	window_tab_text,
	inactive_window_tab,
	inactive_window_tab_text;
private:
	RGBColor *StringToMember(const char *string);
};


bool LoadGUIColors(ColorSet *set);
void SaveGUIColors(const ColorSet &set);

#endif
