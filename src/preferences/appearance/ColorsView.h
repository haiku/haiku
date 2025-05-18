/*
 * Copyright 2002-2025, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef COLORS_VIEW_H_
#define COLORS_VIEW_H_


#include <Button.h>
#include <CheckBox.h>
#include <ColorControl.h>
#include <ColorPreview.h>
#include <ListItem.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include <DecorInfo.h>


class APRWindow;


using BPrivate::BColorPreview;


class ColorsView : public BView {
public:
								ColorsView(const char *name);
	virtual						~ColorsView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *msg);

			void				LoadSettings();

			void				SetDefaults();
			void				Revert();

			bool				IsDefaultable();
			bool				IsRevertable();

private:
			void				_CreateItems();
			void				_UpdatePreviews(const BMessage& colors);

			void				_SetColor(int32 index, rgb_color color);
			void				_SetColor(color_which which, rgb_color color);
			void				_SetOneColor(color_which which, rgb_color color);
			void				_SetCurrentColor(rgb_color color);
			void				_SetUIColors(const BMessage& colors);

private:
			BColorControl*		fPicker;

			BCheckBox*			fAutoSelectCheckBox;
			BListView*			fAttrList;

			color_which			fWhich;

			BScrollView*		fScrollView;

			BColorPreview*		fColorPreview;

			BMessage			fPrevColors;
			BMessage			fDefaultColors;
			BMessage			fCurrentColors;
};

#endif	// COLORS_VIEW_H_
