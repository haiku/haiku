/*

MediaListItem Header by Sikosis

(C)2003

*/

#ifndef __MEDIALISTITEM_H__
#define __MEDIALISTITEM_H__

#include <ListItem.h>
#include <String.h>

class MediaListItem : public BListItem
{
	public:
		MediaListItem(const char* label,int32 MediaName, BBitmap* bitmap,BMessage* msg, uint32 modifiers=0);
		virtual void DrawItem (BView *owner, BRect frame, bool complete = false);
		enum { AS, AM, NI, NO, VS, VWC };
	private:
		BBitmap* icon;
		int32	 kMediaName;
};


#endif
