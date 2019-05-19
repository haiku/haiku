/*
 * Copyright 2012-2019, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under the terms of the MIT licence.
 */


#include <String.h>
#include <View.h>

extern "C" {
	#include <vterm.h>
}


class TermView: public BView
{
	public:
							TermView();
							TermView(BRect bounds);
							~TermView();

				void		AttachedToWindow();
				void		Draw(BRect updateRect);
				void		FrameResized(float width, float height);
				void		GetPreferredSize(float* width, float* height);
				void		KeyDown(const char* bytes, int32 numBytes);
				void		MessageReceived(BMessage* message);
				void		SetLineTerminator(BString bytes);

				void		PushBytes(const char* bytes, const size_t length);
				void		Clear();

	private:
				void		_Init();

				VTermRect	_PixelsToGlyphs(BRect pixels) const;
				BRect		_GlyphsToPixels(const VTermRect& glyphs) const;
				BRect		_GlyphsToPixels(const int width, const int height)
								const;
				void		_GetCell(VTermPos pos, VTermScreenCell& cell);

				void		_Damage(VTermRect rect);
				void		_MoveCursor(VTermPos pos, VTermPos oldPos,
								int visible);
				void		_PushLine(int cols, const VTermScreenCell* cells);
				int			_PopLine(int cols, VTermScreenCell* cells);
				void		_UpdateScrollbar();

		static	int			_Damage(VTermRect rect, void* user);
		static	int			_MoveCursor(VTermPos pos, VTermPos oldPos,
								int visible, void* user);
		static	int			_PushLine(int cols, const VTermScreenCell* cells,
								void* user);
		static	int			_PopLine(int cols, VTermScreenCell* cells,
								void* user);

	private:
		VTerm* fTerm;
		VTermScreen* fTermScreen;
		BList fScrollBuffer;
		int fFontWidth;
		int fFontHeight;

		BString fLineTerminator;

		static const VTermScreenCallbacks sScreenCallbacks;

		static const int kDefaultWidth = 80;
		static const int kDefaultHeight = 25;
		static const int kBorderSpacing = 3;
		static const int kScrollBackSize = 10000;
};
