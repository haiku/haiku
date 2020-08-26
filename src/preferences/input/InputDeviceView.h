/*
 * Copyright 2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Preetpal Kaur <preetpalok123@gmail.com>
 */


#ifndef _INPUT_DEVICE_VIEW_H
#define _INPUT_DEVICE_VIEW_H

#include <ListView.h>
#include <ListItem.h>
#include <Message.h>
#include <StringItem.h>
#include <ScrollBar.h>
#include <String.h>
#include <ScrollView.h>
#include <View.h>


#include "InputIcons.h"
#include "InputTouchpadPref.h"
#include "MouseSettings.h"


#define ITEM_SELECTED 'I1s'

#define kITEM_MARGIN	1
#define GREATER_THAN	-1
#define LESS_THAN	1


class InputIcons;
class TouchpadPref;
class MouseSettings;

enum input_type {
	MOUSE_TYPE,
	TOUCHPAD_TYPE,
	KEYBOARD_TYPE
};

class DeviceListItemView : public BListItem {
public:
						DeviceListItemView(BString title, input_type type);

	void				Update(BView* owner, const BFont* font);
	void				DrawItem(BView* owner, BRect frame,
						bool complete = false);

	const char*			Label() { return fTitle.String();}


	static	InputIcons*	Icons() {return sIcons;}
	static	void		SetIcons(InputIcons* icons) {sIcons = icons;}

protected:
	struct Renderer;

	void				SetRenderParameters(Renderer& renderer);

private:

	static InputIcons*	sIcons;
	BString				fTitle;
	input_type			fInputType;
};


#endif	// _INPUT_DEVICE_VIEW_H */
