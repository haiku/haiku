/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef COLOR_SET_H
#define COLOR_SET_H


#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>


/*!
	\class ColorSet ColorSet.h
	\brief Encapsulates GUI system colors
*/
class ColorSet : public BLocker {
	public:
					ColorSet();
					ColorSet(const ColorSet &cs);
					ColorSet & operator=(const ColorSet &cs);

		void		SetColors(const ColorSet &cs);
		void		PrintToStream(void) const;

		bool		ConvertToMessage(BMessage *msg) const;
		bool		ConvertFromMessage(const BMessage *msg);

		void		SetToDefaults(void);

		rgb_color 	StringToColor(const char *string);
		rgb_color	AttributeToColor(int32 which);

		status_t	SetColor(const char *string, rgb_color value);

		static status_t LoadColorSet(const char *path, ColorSet *set);
		static status_t SaveColorSet(const char *path, const ColorSet &set);

		rgb_color	panel_background,
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
					
					// Not all of these guys do exist in InterfaceDefs.h,
					// but we keep them as part of the color set anyway - 
					// they're important nonetheless
					window_tab_text,
					inactive_window_tab,
					inactive_window_tab_text;

	private:
		rgb_color	*StringToMember(const char *string);
		void		PrintMember(const rgb_color &color) const;
};

#endif	// COLOR_SET_H
