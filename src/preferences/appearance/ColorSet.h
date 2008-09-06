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

typedef struct 
{
	color_which which;
	const char* text;	
} ColorDescription;

const ColorDescription* get_color_description(int32 index);
int32 color_description_count(void);


/*!
	\class ColorSet ColorSet.h
	\brief Encapsulates GUI system colors
*/
class ColorSet : public BLocker {
	public:
					ColorSet();
					ColorSet(const ColorSet &cs);
					ColorSet & operator=(const ColorSet &cs);

		rgb_color	GetColor(int32 which);
		void		SetColor(color_which which, rgb_color value);
		
		static ColorSet	DefaultColorSet(void);
	
		inline bool operator==(const ColorSet &other)
		{
			return fColors == other.fColors;
		}

		inline bool operator!=(const ColorSet &other)
		{
			return fColors != other.fColors;
		}

	private:
		std::map<color_which, rgb_color> fColors;
};

#endif	// COLOR_SET_H
