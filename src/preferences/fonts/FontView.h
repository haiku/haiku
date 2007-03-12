/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_VIEW_H
#define FONT_VIEW_H


#include "FontSelectionView.h"


class FontView : public BView {
	public:
		FontView(BRect frame);

		virtual void GetPreferredSize(float *_width, float *_height);

		void	SetDefaults();
		void	Revert();
		void	UpdateFonts();
		void	RelayoutIfNeeded();

		bool	IsRevertable();

	private:
		FontSelectionView 	*fPlainView;
		FontSelectionView 	*fBoldView;
		FontSelectionView 	*fFixedView;
};
	
#endif	/* FONT_VIEW_H */
