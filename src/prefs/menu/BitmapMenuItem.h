#ifndef _MBitmapMenuItem_h
#define _MBitmapMenuItem_h

// System Headers
#include <Bitmap.h>
#include <MenuItem.h>
#ifndef _TRANSLATION_UTILS_H
#include <TranslationUtils.h>
#endif
#include <String.h>


// MBitmapMenuItem class declaration
class BitmapMenuItem : public BMenuItem
{
public:
					BitmapMenuItem(const char* name, BMessage* message, BBitmap* bmp,
								   char shortcut = 0, uint32 modifiers = 0);
virtual void		Draw(void);
	
private:
	BBitmap 			*fBmp;
	BString				fName;
	BBitmap				*fCheckBmp;
};

#endif // _MBitmapMenuItem_h 