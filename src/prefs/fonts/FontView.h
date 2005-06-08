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
	void	AttachedToWindow(void);
	
	void				resetToDefaults();
	void				revertToOriginal();
	
	FontSelectionView 	*plainSelectionView;
	FontSelectionView 	*boldSelectionView;
	FontSelectionView 	*fixedSelectionView;
};
	
#endif
