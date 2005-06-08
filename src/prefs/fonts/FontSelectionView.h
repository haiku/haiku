/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef FONT_SELECTION_VIEW_H
#define FONT_SELECTION_VIEW_H
	
#include <View.h>
#include <Box.h>
#include <StringView.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <stdio.h>

enum
{
	PLAIN_FONT_SELECTION_VIEW=1,
	BOLD_FONT_SELECTION_VIEW,
	FIXED_FONT_SELECTION_VIEW
};
	
class FontSelectionView : public BView
{
public:
			FontSelectionView(BRect rect, const char *name, int type);
			~FontSelectionView(void);
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);
	
	void	SetTestTextFont(BFont *fnt);
	BFont	GetTestTextFont();
	float	GetSelectedSize();
	void	GetSelectedFont(font_family *family);
	void	GetSelectedStyle(font_style *style);
	void	UpdateFontSelectionFromStyle();
	void	UpdateFontSelection();
	void	resetToDefaults();
	void	revertToOriginal();
	
private:
	void	buildMenus(void);
	void	emptyMenus(void);
	
	void	EmptyMenu(BPopUpMenu *m);
	void	UpdateFontSelection(BFont *fnt);
	
	BStringView		*testText;
	
	BPopUpMenu		*fontList;
	BPopUpMenu		*sizeList;
			
	int				minSizeIndex;
	int				maxSizeIndex;
			
	char typeLabel[30];
			
	BFont origFont;
	BFont workingFont;
	BFont *defaultFont;
};
	
#endif
