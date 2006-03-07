/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPE_WINDOW_H
#define FILE_TYPE_WINDOW_H


#include <Mime.h>
#include <String.h>
#include <Window.h>

#include <ObjectList.h>

class BButton;
class BMenuField;
class BTextControl;

class IconView;
class MimeTypeListView;


class FileTypeWindow : public BWindow {
	public:
		FileTypeWindow(BPoint position, const BMessage& refs);
		virtual ~FileTypeWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BString _Title(const BMessage& refs);
		void _SetTo(const BMessage& refs);
		void _AdoptType(BMessage* message);
		void _AdoptType();
		void _AdoptPreferredApp(BMessage* message, bool sameAs);
		void _AdoptPreferredApp();
		void _UpdatePreferredApps();

	private:
		BObjectList<entry_ref> fEntries;
		BString			fCommonType;
		BString			fCommonPreferredApp;

		BTextControl*	fTypeControl;
		BButton*		fSelectTypeButton;
		BButton*		fSameTypeAsButton;

		IconView*		fIconView;

		BMenuField*		fPreferredField;
		BButton*		fSelectAppButton;
		BButton*		fSameAppAsButton;
};

#endif // FILE_TYPE_WINDOW_H
