/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include <View.h>

extern "C" {
	#include <vterm.h>
}

class TermView: public BView
{
	public:
		TermView();
		~TermView();

		void Draw(BRect updateRect);
		
	private:
		VTermRect PixelsToGlyphs(BRect pixels) const;
		BRect GlyphsToPixels(const VTermRect& glyphs) const;
		BRect GlyphsToPixels(const int width, const int height) const;

	private:
		VTerm* fTerm;
		VTermScreen* fTermScreen;
		float fFontWidth;
		float fFontHeight;

		static const VTermScreenCallbacks sScreenCallbacks;

		static const int kDefaultWidth = 80;
		static const int kDefaultHeight = 25;
		static const int kBorderSpacing = 3;
};
