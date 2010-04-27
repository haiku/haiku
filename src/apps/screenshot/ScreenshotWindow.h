/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 *		Wim van der Meer, WPJvanderMeer@gmail.com
 */
#ifndef SCREENSHOT_WINDOW_H
#define SCREENSHOT_WINDOW_H


#include <String.h>
#include <Window.h>
#include <TranslationDefs.h>
#include <TranslatorFormats.h>


class BBitmap;
class BButton;
class BCardLayout;
class BCheckBox;
class BFilePanel;
class BMenu;
class BRadioButton;
class BTextControl;
class BTextView;
class BPath;
class PreviewView;


class ScreenshotWindow : public BWindow {
public:
							ScreenshotWindow();
							ScreenshotWindow(bigtime_t delay,
								bool includeBorder,
								bool includeMouse,
								bool grabActiveWindow,
								bool showConfigWindow,
								bool saveScreenshotSilent,
								int32 imageFileType = B_PNG_FORMAT,
								const char* outputFilename = NULL);
	virtual					~ScreenshotWindow();

	virtual	void			MessageReceived(BMessage* message);

private:
			void			_InitWindow(const BMessage& settings);
			BPath			_GetDirectory();
			void			_SetupFirstLayoutItem(BCardLayout* layout);
			void			_SetupSecondLayoutItem(BCardLayout* layout,
								const BMessage& settings);
			void			_DisallowChar(BTextView* textView);
			void			_SetupTranslatorMenu(BMenu* translatorMenu);
			void			_SetupOutputPathMenu(BMenu* outputPathMenu,
								const BMessage& settings);
			void			_AddItemToPathMenu(const char* path,
								BString& label, int32 index, bool markItem);

			void			_UpdatePreviewPanel();
			void			_UpdateFilenameSelection();
			BString			_FindValidFileName(const char* name);
			int32			_PathIndexInMenu(const BString& path) const;

			BMessage		_ReadSettings() const;
			void			_WriteSettings() const;

			void			_TakeScreenshot(bigtime_t delay);
			status_t		_GetActiveWindowFrame(BRect* frame);
			void			_MakeTabSpaceTransparent(BRect* frame);

			status_t		_SaveScreenshot();

			status_t		_FindTranslator(uint32 imageType, translator_id* id);

			PreviewView*	fPreview;
			BRadioButton*	fActiveWindow;
			BRadioButton*	fWholeDesktop;
			BTextControl*	fDelayControl;
			BCheckBox*		fWindowBorder;
			BCheckBox*		fShowMouse;
			BButton*		fBackToSave;
			BButton*		fTakeScreenshot;
			BButton*		fSaveScreenshot;
			BTextControl*	fNameControl;
			BMenu*			fTranslatorMenu;
			BMenu*			fOutputPathMenu;
			BBitmap*		fScreenshot;
			BFilePanel*		fOutputPathPanel;
			BMenuItem*		fLastSelectedPath;

			bigtime_t		fDelay;
			float			fTabHeight;

			bool			fIncludeBorder;
			bool			fIncludeMouse;
			bool			fGrabActiveWindow;
			bool			fShowConfigWindow;
			bool			fSaveScreenshotSilent;
			BString			fOutputFilename;
			BString			fExtension;

			int32			fImageFileType;
};

#endif	/* SCREENSHOT_WINDOW_H */
