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
#include <String.h>

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
	
	void	SetDefaults(void);
	void	Revert(void);
	void	RescanFonts(void);

private:
	void	BuildMenus(void);
	void	EmptyMenus(void);
	void	NotifyFontChange(void);
	
	BStringView		*fPreviewText;
	
	BPopUpMenu		*fFontMenu;
	BPopUpMenu		*fSizeMenu;
			
	int				fType;
	
	BFont			fSavedFont;
	BFont			fCurrentFont;
	BFont			fDefaultFont;
	
	BMenuItem		*fCurrentStyle;
};
	
#endif
