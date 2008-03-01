/*
 * Copyright 2001-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Rene Gollent <rene@gollent.com>
 */
#ifndef COLOR_SET_H
#define COLOR_SET_H


#include <InterfaceDefs.h>
#include <Locker.h>
#include <Message.h>
#include <String.h>

#include <map>

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

		bool		IsDefaultable(void);

		rgb_color 	StringToColor(const char *string) const;
		rgb_color	AttributeToColor(int32 which);

		status_t	SetColor(color_which which, rgb_color value);

		static status_t LoadColorSet(const char *path, ColorSet *set);
		static status_t SaveColorSet(const char *path, const ColorSet &set);
		
		static ColorSet	DefaultColorSet(void);
		static std::map<color_which, BString> DefaultColorNames(void);
	
		inline bool operator==(const ColorSet &other)
		{
			return (fColors == other.fColors);
		}

	private:
		color_which	StringToWhich(const char *string) const;
		void		PrintMember(color_which which) const;
		std::map<color_which, rgb_color> fColors;
};

#endif	// COLOR_SET_H
