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

class Utility;


class ScreenshotWindow : public BWindow {
public:
							ScreenshotWindow(const Utility& utility,
								bool silent, bool clipboard);
							~ScreenshotWindow();

			void			MessageReceived(BMessage* message);
			void			Quit();

private:
			void			_NewScreenshot(bool silent = false,
								bool clipboard = false);
			void			_UpdatePreviewPanel();
			void			_DisallowChar(BTextView* textView);
			void			_SetupOutputPathMenu(const BMessage& settings);
			void			_AddItemToPathMenu(const char* path,
								BString& label, int32 index, bool markItem);
			void			_UpdateFilenameSelection();
			void			_SetupTranslatorMenu();
			status_t		_SaveScreenshot();
			void			_ShowSettings(bool activate);
			BString			_FindValidFileName(const char* name);
			BPath			_GetDirectory();
			void			_ReadSettings();
			void			_WriteSettings();

	const	Utility&		fUtility;

			BView*			fPreview;
			BCheckBox*		fActiveWindow;
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

			bigtime_t		fDelay;
			bool			fIncludeBorder;
			bool			fIncludeCursor;
			bool			fGrabActiveWindow;
			BString			fOutputFilename;
			BString			fExtension;
			int32			fImageFileType;
};


#endif // SCREENSHOT_WINDOW_H
