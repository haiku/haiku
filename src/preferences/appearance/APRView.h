/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
 *		Rene Gollent (rene@gollent.com)
 */
#ifndef APR_VIEW_H_
#define APR_VIEW_H_

#include <View.h>
#include <ColorControl.h>
#include <Message.h>
#include <ListItem.h>
#include <ListView.h>
#include <Button.h>
#include <ScrollView.h>
#include <ScrollBar.h>
#include <String.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <Invoker.h>

#include "ColorSet.h"

class ColorWell;
class APRWindow;

class APRView : public BView
{
public:
			APRView(const char *name, uint32 flags);
			~APRView(void);
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);
	
	void	LoadSettings(void);
	bool	IsDefaultable(void);

protected:

	void	SetCurrentColor(rgb_color color);
	void	UpdateControls();
	void	UpdateAllColors();
	
	BColorControl	*fPicker;
	
	BListView		*fAttrList;
	
	color_which		fWhich;
	
	BScrollView		*fScrollView;
	
	ColorWell		*fColorWell;

	ColorSet		fCurrentSet;
	ColorSet		fPrevSet;
	ColorSet		fDefaultSet;
	
	BMenu			*fDecorMenu;
};

#endif
