#ifndef _THEME_ITEM_H_
#define _THEME_ITEM_H_

#include <ListItem.h>


class ThemeItem : public BStringItem
{
public:
					ThemeItem(int32 id, const char *name, bool ro = false);
					~ThemeItem();
		void		DrawItem(BView *owner, BRect frame, bool complete = false);
		bool		IsCurrent();
		void		SetCurrent(bool set);
		bool		IsReadOnly();
		void		SetReadOnly(bool set);
		int32		ThemeId();
private:
		int32		fId;
		bool		fRo;
		bool		fCurrent;
};

#endif // _THEME_ITEM_H_
