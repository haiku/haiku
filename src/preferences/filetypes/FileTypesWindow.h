/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPES_WINDOW_H
#define FILE_TYPES_WINDOW_H


#include <Window.h>

class BButton;
class BListView;
class BMenuField;
class BMimeType;
class BOutlineListView;
class BTextControl;

class AttributeListView;
class IconView;


class FileTypesWindow : public BWindow {
	public:
		FileTypesWindow(BRect frame);
		virtual ~FileTypesWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		void _UpdateExtensions(BMimeType* type);
		void _AddSignature(BMenuItem* item, const char* signature);
		void _UpdatePreferredApps(BMimeType* type);
		void _SetType(BMimeType* type);

	private:
		BOutlineListView* fTypeListView;
		BButton*		fRemoveTypeButton;

		IconView*		fIconView;

		BListView*		fExtensionListView;
		BButton*		fAddExtensionButton;
		BButton*		fRemoveExtensionButton;

		BTextControl*	fInternalNameControl;
		BTextControl*	fTypeNameControl;

		BMenuField*		fPreferredField;
		BButton*		fSelectButton;
		BButton*		fSameAsButton;

		AttributeListView* fAttributeListView;
		BButton*		fAddAttributeButton;
		BButton*		fRemoveAttributeButton;

};

#endif	// FILE_TYPES_WINDOW_H
