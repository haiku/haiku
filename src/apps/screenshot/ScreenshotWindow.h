/*
 * Copyright 2010 Wim van der Meer <WPJvanderMeer@gmail.com>
 * Copyright Karsten Heimrich, host.haiku@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 *		Wim van der Meer
 */
#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H


#include "Rect.h"
#include "Utility.h"
#include <RadioButton.h>
#include <String.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>
#include <Window.h>


class BBitmap;
class BCheckBox;
class BFilePanel;
class BMenu;
class BPath;
class BTextControl;
class BTextView;


class ScreenshotWindow : public BWindow {
public:
							ScreenshotWindow(const Utility& utility, bool silent,
								bool clipboard);
							~ScreenshotWindow();

			void			MessageReceived(BMessage* message);
			void			Quit();
			void			SetSelectedArea(BRect frame);

private:
			void			_NewScreenshot(bool silent = false,
								bool clipboard = false,
								bool ignoreDelay = false,
								bool selectArea = false);
			void			_UpdatePreviewPanel();
			void			_DisallowChar(BTextView* textView);
			void			_SetupOutputPathMenu(const BMessage& settings);
			void			_AddItemToPathMenu(const char* path,
								BString& label, int32 index, bool markItem,
								uint32 shortcutKey = 0);
			void			_UpdateFilenameSelection();
			void			_SetupTranslatorMenu();
			void			_DisplaySaveError(BString _message);
			status_t		_SaveScreenshot();
			void			_ShowSettings(bool activate);
			BString			_FindValidFileName(const char* name);
			BPath			_GetDirectory();
			void			_ReadSettings();
			void			_WriteSettings();

	const	Utility&		fUtility;

			BView*			fPreview;
			BRadioButton*	fActiveWindow;
			BRadioButton*	fAreaSelect;
			BRadioButton*   fWholeScreen;
			BTextControl*	fDelayControl;
			BCheckBox*		fWindowBorder;
			BCheckBox*		fShowCursor;
			BTextControl*	fNameControl;
			BMenu*			fTranslatorMenu;
			BMenu*			fOutputPathMenu;
			BBitmap*		fScreenshot;
			BFilePanel*		fOutputPathPanel;
			BMenuItem*		fLastSelectedPath;
			BWindow*		fSettingsWindow;
			BWindow* 		fSelectAreaWindow;

			bigtime_t		fDelay;
			bool			fIncludeBorder;
			bool			fIncludeCursor;
			ShotType		fShotType;
			BString			fOutputFilename;
			BString			fExtension;
			int32			fImageFileType;
			BRect			fSelectedArea;
};


#endif // SCREENSHOT_WINDOW_H
