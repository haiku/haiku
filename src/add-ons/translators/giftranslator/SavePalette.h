////////////////////////////////////////////////////////////////////////////////
//
//	File: SavePalette.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SAVE_PALETTE_H
#define SAVE_PALETTE_H

#include "SFHash.h"
#include <GraphicsDefs.h>
class BBitmap;

enum {
	WEB_SAFE_PALETTE = 0,
	BEOS_SYSTEM_PALETTE,
	GREYSCALE_PALETTE,
	OPTIMAL_PALETTE
};

class SavePalette {
	public:
							SavePalette(int mode);
							SavePalette(BBitmap *bitmap,
										int32 maxSizeInBits = 8);
	virtual					~SavePalette();
		
	inline	bool			IsValid() const
								{ return !fFatalError; }

			uint8			IndexForColor(uint8 red, uint8 green, uint8 blue);
	inline	uint8			IndexForColor(const rgb_color& color);

			void			SetUseTransparent(bool use);
	inline	bool			UseTransparent() const
								{ return fUseTransparent; }

			void			SetTransparentIndex(int index);
	inline	int				TransparentIndex() const
								{ return fTransparentIndex; }
			bool			SetTransparentColor(uint8 red,
												uint8 green,
												uint8 blue);

	inline	int				SizeInBits() const
								{ return fSizeInBits; }

	inline	int				BackgroundIndex() const
								{ return fBackgroundIndex; }

			void			GetColors(uint8* buffer, int size) const;

			rgb_color*		pal;

	private:
			int				fSize;
			int				fSizeInBits;
			int				fMode;
			bool			fUseTransparent;
			int				fTransparentIndex;
			int				fBackgroundIndex;
			bool			fFatalError;
};

// IndexForColor
inline uint8
SavePalette::IndexForColor(const rgb_color& color)
{
	return IndexForColor(color.red, color.green, color.blue);
}


#endif


