/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef EDIT_WINDOW_H
#define EDIT_WINDOW_H


#include <Window.h>


class EditView;
class ResourceRow;

class BBox;
class BButton;
class BMenuField;
class BPopUpMenu;
class BStringView;
class BTextControl;


class EditWindow : public BWindow {
public:
					EditWindow(BRect frame, ResourceRow* row);
					~EditWindow();

	void			MessageReceived(BMessage* msg);

private:
	ResourceRow*	fRow;

	BView*			fView;

	BTextControl*	fIDText;
	BTextControl*	fNameText;
	BPopUpMenu*		fTypePopUp;
	BMenuField*		fTypeMenu;
	BTextControl*	fCodeText;

	EditView*		fEditView;
	BBox*			fEditViewBox;

	BStringView*	fErrorString;

	BButton*		fCancelButton;
	BButton*		fOKButton;

	bool			_Validate();
};


#endif
