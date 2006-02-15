/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPES_WINDOW_H
#define FILE_TYPES_WINDOW_H


#include <Alert.h>
#include <Mime.h>
#include <Window.h>

class BButton;
class BListView;
class BMenuField;
class BMimeType;
class BOutlineListView;
class BTextControl;

class AttributeListView;
class IconView;
class MimeTypeListView;
class StringView;


class FileTypesWindow : public BWindow {
	public:
		FileTypesWindow(BRect frame);
		virtual ~FileTypesWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

		void PlaceSubWindow(BWindow* window);

	private:
		void _UpdateExtensions(BMimeType* type);
		void _AdoptPreferredApplication(BMessage* message, bool sameAs);
		void _AddSignature(BMenuItem* item, const char* signature);
		BMenuItem* _CreateApplicationItem(const char* signature);
		void _UpdatePreferredApps(BMimeType* type);
		void _UpdateIcon(BMimeType* type);
		void _SetType(BMimeType* type, int32 forceUpdate = 0);

	private:
		BMimeType		fCurrentType;

		MimeTypeListView* fTypeListView;
		BButton*		fRemoveTypeButton;

		IconView*		fIconView;

		BListView*		fExtensionListView;
		BButton*		fAddExtensionButton;
		BButton*		fRemoveExtensionButton;

		StringView*		fInternalNameView;
		BTextControl*	fTypeNameControl;
		BTextControl*	fDescriptionControl;

		BMenuField*		fPreferredField;
		BButton*		fSelectButton;
		BButton*		fSameAsButton;

		AttributeListView* fAttributeListView;
		BButton*		fAddAttributeButton;
		BButton*		fRemoveAttributeButton;

		BWindow*		fNewTypeWindow;
};

static const uint32 kMsgPreferredAppOpened = 'paOp';
static const uint32 kMsgSamePreferredAppAsOpened = 'spaO';

static const uint32 kMsgSelectNewType = 'slnt';
static const uint32 kMsgNewTypeWindowClosed = 'ntwc';


extern void error_alert(const char* message, status_t status = B_OK,
	alert_type type = B_WARNING_ALERT);

#endif	// FILE_TYPES_WINDOW_H
