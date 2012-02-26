/*
 * Copyright 2002-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef APR_VIEW_H_
#define APR_VIEW_H_


#include <Button.h>
#include <ColorControl.h>
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

#include "ColorSet.h"


class ColorWell;
class APRWindow;


class APRView : public BView {
public:
								APRView(const char *name);
	virtual						~APRView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *msg);

			void				LoadSettings();

			void				SetDefaults();
			void				Revert();

			bool				IsDefaultable();
			bool				IsRevertable();

private:
			void				_SetCurrentColor(rgb_color color);
			void				_UpdateControls();
			void				_UpdateAllColors();

private:
			BColorControl*		fPicker;

			BListView*			fAttrList;

			color_which			fWhich;

			BScrollView*		fScrollView;

			ColorWell*			fColorWell;

			ColorSet			fCurrentSet;
			ColorSet			fPrevSet;
			ColorSet			fDefaultSet;
};

#endif
