/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm (darkwyrm@earthlink.net)
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
			APRView(const BRect &frame, const char *name, int32 resize, 
					int32 flags);
			~APRView(void);
	void	AttachedToWindow(void);
	void	MessageReceived(BMessage *msg);
	
	void	LoadSettings(void);

protected:

	void	UpdateControlsFromAttr(const char *string);
	
	BColorControl	*fPicker;
	
	BButton			*fRevert;
	BButton			*fDefaults;
	
	BListView		*fAttrList;
	
	color_which		fAttribute;
	
	BString			fAttrString;
	
	BScrollView		*fScrollView;
	
	ColorWell		*fColorWell;

	ColorSet		fCurrentSet;
	ColorSet		fPrevSet;
};

#endif
