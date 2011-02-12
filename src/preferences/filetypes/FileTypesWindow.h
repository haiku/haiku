/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef FILE_TYPES_WINDOW_H
#define FILE_TYPES_WINDOW_H


#include <Alert.h>
#include <Mime.h>
#include <Window.h>


class BBox;
class BButton;
class BListView;
class BMenuField;
class BMimeType;
class BOutlineListView;
class BSplitView;
class BTextControl;

class AttributeListView;
class ExtensionListView;
class MimeTypeListView;
class StringView;
class TypeIconView;


static const uint32 kMsgSelectNewType = 'slnt';
static const uint32 kMsgNewTypeWindowClosed = 'ntwc';


class FileTypesWindow : public BWindow {
public:
								FileTypesWindow(const BMessage& settings);
	virtual						~FileTypesWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

			void				SelectType(const char* type);

			void				PlaceSubWindow(BWindow* window);

private:
			BRect				_Frame(const BMessage& settings) const;
			void				_ShowSnifferRule(bool show);
			void				_UpdateExtensions(BMimeType* type);
			void				_AdoptPreferredApplication(BMessage* message,
									bool sameAs);
			void				_UpdatePreferredApps(BMimeType* type);
			void				_UpdateIcon(BMimeType* type);
			void				_SetType(BMimeType* type,
									int32 forceUpdate = 0);
			void				_MoveUpAttributeIndex(int32 index);

private:
			BMimeType			fCurrentType;

			BSplitView*			fMainSplitView;

			MimeTypeListView*	fTypeListView;
			BButton*			fRemoveTypeButton;

			BBox*				fIconBox;
			TypeIconView*		fIconView;

			BBox*				fRecognitionBox;
			StringView*			fExtensionLabel;
			ExtensionListView*	fExtensionListView;
			BButton*			fAddExtensionButton;
			BButton*			fRemoveExtensionButton;
			BTextControl*		fRuleControl;

			BBox*				fDescriptionBox;
			StringView*			fInternalNameView;
			BTextControl*		fTypeNameControl;
			BTextControl*		fDescriptionControl;

			BBox*				fPreferredBox;
			BMenuField*			fPreferredField;
			BButton*			fSelectButton;
			BButton*			fSameAsButton;

			BBox*				fAttributeBox;
			AttributeListView*	fAttributeListView;
			BButton*			fAddAttributeButton;
			BButton*			fRemoveAttributeButton;
			BButton*			fMoveUpAttributeButton;
			BButton*			fMoveDownAttributeButton;

			BWindow*			fNewTypeWindow;
};


#endif	// FILE_TYPES_WINDOW_H
