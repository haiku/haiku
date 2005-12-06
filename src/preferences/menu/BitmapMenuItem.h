#ifndef _MBitmapMenuItem_h
#define _MBitmapMenuItem_h

#include <MenuItem.h>
#include <String.h>

class BBitmap;

// MBitmapMenuItem class declaration
class BitmapMenuItem : public BMenuItem {
public:
				BitmapMenuItem(const char* name, BMessage* message, BBitmap* bmp,
							   char shortcut = 0, uint32 modifiers = 0);
				~BitmapMenuItem();
	virtual void		DrawContent();
	
private:
	BBitmap 		*fBitmap;
	BString			fName;
};

#endif // _MBitmapMenuItem_h
