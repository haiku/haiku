#ifndef _VIEW_BUFFER_H_
#define _VIEW_BUFFER_H_

#include <SupportDefs.h>
#include <View.h>

typedef void (*resize_callback)(int32 width, int32 height, void *data);

class ViewBuffer : public BView {
public:
							ViewBuffer(BRect frame);
virtual						~ViewBuffer();

virtual	void				FrameResized(float new_width, float new_height);
		void				SetResizeCallback(resize_callback callback, void *data);
		status_t			GetSize(int32 *width, int32 *height);

		uint8				ForegroundColor(uint8 attr);
		uint8				BackgroundColor(uint8 attr);
		rgb_color			GetPaletteEntry(uint8 index);

		void				PutGlyph(int32 x, int32 y, uint8 glyph, uint8 attr);
		void				FillGlyph(int32 x, int32 y, int32 width, int32 height, uint8 glyph, uint8 attr);
		void				RenderGlyph(int32 x, int32 y, uint8 glyph, uint8 attr);

virtual	void				Draw(BRect updateRect);

		void				DrawCursor(int32 x, int32 y);
		void				MoveCursor(int32 x, int32 y);

		void				Blit(int32 srcx, int32 srcy, int32 width, int32 height, int32 destx, int32 desty);
		void				Clear(uint8 attr);

private:
		void				_RenderGlyph(int32 x, int32 y, const char* string, uint8 attr, bool fill = true);

		int32				fColumns;
		int32				fRows;

		uint16*				fGlyphGrid;

		resize_callback		fResizeCallback;
		void				*fResizeCallbackData;
		int32				fCursorX;
		int32				fCursorY;

		rgb_color			fPalette[8];
};

#endif
