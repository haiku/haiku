/*
 * Copyright 2001-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef FONT_VIEW_H
#define FONT_VIEW_H


#include <View.h>


class BMessageRunner;
class FontSelectionView;


class FontView : public BView {
public:
								FontView(const char* name);

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();

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

			BMessageRunner*		fRunner;
};


#endif	/* FONT_VIEW_H */
