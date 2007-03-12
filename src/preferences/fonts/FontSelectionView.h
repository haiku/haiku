/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef FONT_SELECTION_VIEW_H
#define FONT_SELECTION_VIEW_H


#include <View.h>

class BBox;
class BMenuField;
class BPopUpMenu;
class BStringView;


class FontSelectionView : public BView {
	public:
		FontSelectionView(BRect rect, const char* name, const char* label,
			const BFont& currentFont);
		virtual ~FontSelectionView();

		virtual void	GetPreferredSize(float *_width, float *_height);
		virtual void	AttachedToWindow();
		virtual void	MessageReceived(BMessage *msg);

		void			SetDivider(float divider);
		void			RelayoutIfNeeded();

		void			SetDefaults();
		void			Revert();
		bool			IsRevertable();

		void			UpdateFontsMenu();

	private:
		void			_SelectCurrentFont(bool select);
		void			_SelectCurrentSize(bool select);
		void			_UpdateFontPreview();
		void			_UpdateSystemFont();
		void			_BuildSizesMenu();

	protected:
		float			fDivider;

		BMenuField*		fFontsMenuField;
		BMenuField*		fSizesMenuField;
		BPopUpMenu*		fFontsMenu;
		BPopUpMenu*		fSizesMenu;

		BBox*			fPreviewBox;
		BStringView*	fPreviewText;

		BFont			fSavedFont;
		BFont			fCurrentFont;
		float			fMaxFontNameWidth;
};
	
#endif	/* FONT_SELECTION_VIEW_H */
