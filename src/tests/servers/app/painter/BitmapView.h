// BitmapView.h

#ifndef BITMAP_VIEW_H
#define BITMAP_VIEW_H

#include <View.h>

class BBitmap;

class BitmapView : public BView {
 public:
								BitmapView(BRect frame,
										   const char* name,
										   BBitmap* bitmap);
	virtual						~BitmapView();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

 private:
			BBitmap*			fBitmap;
};


#endif // BITMAP_VIEW_H
