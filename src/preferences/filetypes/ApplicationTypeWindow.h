/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef APPLICATION_TYPE_WINDOW_H
#define APPLICATION_TYPE_WINDOW_H


#include "IconView.h"

#include <AppFileInfo.h>
#include <Mime.h>
#include <String.h>
#include <Window.h>

class BButton;
class BCheckBox;
class BListView;
class BPopUpMenu;
class BRadioButton;
class BTextControl;
class BTextView;

class MimeTypeListView;


class ApplicationTypeWindow : public BWindow {
	public:
		ApplicationTypeWindow(BPoint position, const BEntry& entry);
		virtual ~ApplicationTypeWindow();

		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BString _Title(const BEntry& entry);
		void _SetTo(const BEntry& entry);
		void _UpdateAppFlagsEnabled();
		void _MakeNumberTextControl(BTextControl* control);
		void _Save();

		bool _Flags(uint32& flags) const;
		BMessage _SupportedTypes() const;
		version_info _VersionInfo() const;

		void _CheckSaveMenuItem(uint32 flags);
		uint32 _NeedsSaving(uint32 flags) const;

	private:
		struct AppInfo {
			BString			signature;
			bool			gotFlags;
			uint32			flags;
			version_info	versionInfo;

			BMessage		supportedTypes;

			bool			iconChanged;
			bool			typeIconsChanged;
		};
		enum {
			CHECK_SIGNATUR			= 1 << 0,
			CHECK_FLAGS				= 1 << 1,
			CHECK_VERSION			= 1 << 2,
			CHECK_ICON				= 1 << 3,

			CHECK_TYPES				= 1 << 4,
			CHECK_TYPE_ICONS		= 1 << 5,

			CHECK_ALL				= 0xffffffff
		};

	private:
		BEntry			fEntry;
		AppInfo			fOriginalInfo;

		BTextControl*	fSignatureControl;
		IconView*		fIconView;
		Icon			fIcon;

		BCheckBox*		fFlagsCheckBox;
		BRadioButton*	fSingleLaunchButton;
		BRadioButton*	fMultipleLaunchButton;
		BRadioButton*	fExclusiveLaunchButton;
		BCheckBox*		fArgsOnlyCheckBox;
		BCheckBox*		fBackgroundAppCheckBox;

		BListView*		fTypeListView;
		BButton*		fAddTypeButton;
		BButton*		fRemoveTypeButton;
		IconView*		fTypeIconView;

		BTextControl*	fMajorVersionControl;
		BTextControl*	fMiddleVersionControl;
		BTextControl*	fMinorVersionControl;
		BPopUpMenu*		fVarietyMenu;
		BTextControl*	fInternalVersionControl;
		BTextControl*	fShortDescriptionControl;
		BTextView*		fLongDescriptionView;

		BMenuItem*		fSaveMenuItem;
		uint32			fChangedProperties;
};

#endif // APPLICATION_TYPE_WINDOW_H
