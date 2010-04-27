/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef APPLICATION_TYPES_WINDOW_H
#define APPLICATION_TYPES_WINDOW_H


#include <Alert.h>
#include <Mime.h>
#include <Window.h>

class BButton;
class BListView;
class BMenuField;
class BMimeType;
class BOutlineListView;
class BStringView;
class BTextView;

class MimeTypeListView;
class StringView;


class ApplicationTypesWindow : public BWindow {
	public:
		ApplicationTypesWindow(const BMessage& settings);
		virtual ~ApplicationTypesWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BRect _Frame(const BMessage& settings) const;
		void _SetType(BMimeType* type, int32 forceUpdate = 0);
		void _UpdateCounter();
		void _RemoveUninstalled();

	private:
		BMimeType		fCurrentType;

		MimeTypeListView* fTypeListView;
		BButton*		fRemoveTypeButton;

		StringView*		fNameView;
		StringView*		fSignatureView;
		StringView*		fPathView;

		StringView*		fVersionView;
		StringView*		fDescriptionLabel;
		BTextView*		fDescriptionView;

		BButton*		fTrackerButton;
		BButton*		fLaunchButton;
		BButton*		fEditButton;
};

#endif	// APPLICATION_TYPES_WINDOW_H
