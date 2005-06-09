/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef FONT_VIEW_H
#define FONT_VIEW_H

#include <Box.h>
#include <View.h>
#include "FontSelectionView.h"

class FontView : public BView
{
public:
			FontView(BRect frame); 
	void	SetDefaults(void);
	void	Revert(void);
	void	RescanFonts(void);

private:

	FontSelectionView 	*fPlainView;
	FontSelectionView 	*fBoldView;
	FontSelectionView 	*fFixedView;
};
	
#endif
