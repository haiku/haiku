/*************************************************************
*   IconMenu
*
*   Toolbar is a usefull UI component.
*
*   @author  Atsushi Takamatsu (tak_atsu@tau.bekkoame.ne.jp)
**************************************************************/

#ifndef __ICONMENUITEM_H__
#define __ICONMENUITEM_H__

#include <MenuItem.h>

class IconMenuItem :public BMenuItem {
public:
					IconMenuItem(const char* label
								,BMessage *message
								,char shortcut = 0
								,uint32 modifiers = 0
								,BBitmap *bitmap = NULL
								,bool copyBitmap = true
								,bool deleteInputBitmap = true);
					IconMenuItem(BMenu *submenu
								,BMessage *message
								,char shortcut = 0
								,uint32 modifiers = 0
								,BBitmap *bitmap = NULL
								,bool copyBitmap = true
								,bool deleteInputBitmap = true);
		virtual		~IconMenuItem();
			void	SetBitmap(BBitmap *bitmap,bool freePointer=false);
protected:
	virtual	void	DrawContent();
	virtual	void	GetContentSize(float *width ,float *height);
private:
		BBitmap 	*fBitmap;
		bool		fCopy;
		float 		fHeightDelta;
};
#endif
