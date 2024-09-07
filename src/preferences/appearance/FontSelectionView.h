/*
 * Copyright 2001-2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mark Hogben
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Philippe Saint-Pierre, stpere@gmail.com
 */
#ifndef FONT_SELECTION_VIEW_H
#define FONT_SELECTION_VIEW_H


#include <View.h>

class BLayoutItem;
class BBox;
class BMenuField;
class BPopUpMenu;
class BSpinner;
class BTextView;

static const int32 kMsgSetFamily = 'fmly';
static const int32 kMsgSetStyle = 'styl';
static const int32 kMsgSetSize = 'size';


class FontSelectionView : public BView {
public:
								FontSelectionView(const char* name,
									const char* label,
									const BFont* font = NULL);
	virtual						~FontSelectionView();

	virtual void				MessageReceived(BMessage* message);

			void				SetTarget(BHandler* messageTarget);

			void				SetDefaults();
			void				Revert();
			bool				IsDefaultable();
			bool				IsRevertable();

			void				UpdateFontsMenu();

private:
			void				_SelectCurrentFont(bool select);
			void				_SelectCurrentSize();
			void				_UpdateFontPreview();
			void				_UpdateSystemFont();
			void				_BuildSizesMenu();

protected:
			BHandler*			fMessageTarget;

			BMenuField*			fFontsMenuField;
			BPopUpMenu*			fFontsMenu;

			BSpinner*			fFontSizeSpinner;

			BBox*				fPreviewBox;
			BTextView*			fPreviewTextView;
			float				fPreviewTextWidth;

			BFont				fSavedFont;
			BFont				fCurrentFont;
};

#endif	// FONT_SELECTION_VIEW_H
