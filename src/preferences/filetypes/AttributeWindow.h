/*
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_WINDOW_H
#define ATTRIBUTE_WINDOW_H


#include "AttributeListView.h"

#include <Messenger.h>
#include <Mime.h>
#include <String.h>
#include <Window.h>

class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BTextControl;

class FileTypesWindow;


class AttributeWindow : public BWindow {
	public:
		AttributeWindow(FileTypesWindow* target, BMimeType& type,
			AttributeItem* item);
		virtual ~AttributeWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		type_code _CurrentType() const;
		BMenuItem* _DefaultDisplayAs() const;
		void _CheckDisplayAs();
		void _CheckAcceptable();
		AttributeItem* _NewItemFromCurrent();

	private:
		BMessenger		fTarget;
		BMimeType		fMimeType;
		AttributeItem	fAttribute;
		BTextControl*	fPublicNameControl;
		BTextControl*	fAttributeControl;
		BMenu*			fTypeMenu;
		BMenuField*		fDisplayAsMenuField;
		BMenuField*		fAlignmentMenuField;
		BCheckBox*		fVisibleCheckBox;
		BCheckBox*		fEditableCheckBox;
		BTextControl*	fSpecialControl;
		BTextControl*	fWidthControl;
		BButton*		fAcceptButton;
};

#endif	// ATTRIBUTE_WINDOW_H
