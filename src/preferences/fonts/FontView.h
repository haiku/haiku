/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Saint-Pierre, stpere@gmail.com
 */
#ifndef FONT_VIEW_H
#define FONT_VIEW_H

#include "FontSelectionView.h"
#include "MainWindow.h"
#include <Message.h>

class FontView : public BView {
public:
								FontView();

	virtual void				MessageReceived(BMessage* message);

			void				SetDefaults();
			void				Revert();
			void				UpdateFonts();

			bool				IsDefaultable();
			bool				IsRevertable();



private:
			FontSelectionView*	fPlainView;
			FontSelectionView*	fBoldView;
			FontSelectionView*	fFixedView;
			FontSelectionView*	fMenuView;
};


#endif	/* FONT_VIEW_H */
