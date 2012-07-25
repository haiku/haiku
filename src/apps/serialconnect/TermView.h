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

		void AttachedToWindow();
		void Draw(BRect updateRect);
		void GetPreferredSize(float* width, float* height);
		void KeyDown(const char* bytes, int32 numBytes);
		void PushBytes(const char* bytes, const size_t length);
		
	private:
		VTermRect PixelsToGlyphs(BRect pixels) const;
		BRect GlyphsToPixels(const VTermRect& glyphs) const;
		BRect GlyphsToPixels(const int width, const int height) const;
		void Damage(VTermRect rect);

		static int Damage(VTermRect rect, void* user);

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
