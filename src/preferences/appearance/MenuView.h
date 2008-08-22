/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>		
 */
#ifndef MENUVIEW_H_
#define MENUVIEW_H_

#include <Menu.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <View.h>
#include <Button.h>
#include <RadioButton.h>
#include <Box.h>
#include <CheckBox.h>
#include "FontMenu.h"

class MenuView : public BView
{
public:
			MenuView(BRect frame, const char *name, int32 resize, int32 flags);
			~MenuView(void);
	void 	AttachedToWindow(void);
	void 	MessageReceived(BMessage *msg);
	void 	MarkCommandKey(void);
	void 	SetCommandKey(bool use_alt);
	
private:
	BMenuField 		*fontfield, *fontsizefield;
	BMenu 			*fontsizemenu;
	FontMenu 		*fontmenu;
	BButton 		*defaultsbutton, *revertbutton;
	BCheckBox 		*showtriggers;
	BRadioButton 	*altcmd, *ctrlcmd;
	menu_info 		menuinfo,revertinfo;
	bool 			altcommand;
	bool 			revert, revertaltcmd;
};

#endif
