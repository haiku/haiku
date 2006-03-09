/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef APPLICATION_TYPE_WINDOW_H
#define APPLICATION_TYPE_WINDOW_H


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

class IconView;
class MimeTypeListView;


class ApplicationTypeWindow : public BWindow {
	public:
		ApplicationTypeWindow(BPoint position, const BEntry& entry);
		virtual ~ApplicationTypeWindow();

		virtual void FrameResized(float width, float height);
		virtual void MessageReceived(BMessage* message);
		virtual bool QuitRequested();

	private:
		BString _Title(const BEntry& entry);
		void _SetTo(const BEntry& entry);
		void _UpdateAppFlagsEnabled();
		void _MakeNumberTextControl(BTextControl* control);
		void _Save();

	private:
		BEntry			fEntry;

		BTextControl*	fSignatureControl;
		IconView*		fIconView;

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
};

#endif // APPLICATION_TYPE_WINDOW_H
