#ifndef __HBITMAPVIEW_H__
#define __HBITMAPVIEW_H__

#include <View.h>
#include <Bitmap.h>

class BitmapView :public BView {
public:
				BitmapView(BRect rect,
							const char* name,
							uint32 resizing_mode,
							BBitmap *bitmap,
							rgb_color bgcolor = ui_color(B_PANEL_BACKGROUND_COLOR));
virtual 		~BitmapView();
virtual void	Draw(BRect rect);
		void	SetBitmap(BBitmap *bitmap);
protected:
		BBitmap *fBitmap;
};
#endif