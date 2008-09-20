/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <String.h>
#include <Window.h>


class BBitmap;
class BBox;
class BButton;
class BCardLayout;
class BCheckBox;
class BFilePanel;
class BMenu;
class BRadioButton;
class BTextControl;
class BTextView;


class ScreenshotWindow : public BWindow {
public:
							ScreenshotWindow(bigtime_t delay = 0,
								bool includeBorder = false,
								bool includeCursor = false,
								bool grabActiveWindow = false,
								bool showConfigWindow = false,
								bool saveScreenshotSilent = false);
	virtual					~ScreenshotWindow();

	virtual void			MessageReceived(BMessage* message);

private:
			void			_InitWindow();
			void			_SetupFirstLayoutItem(BCardLayout* layout);
			void			_SetupSecondLayoutItem(BCardLayout* layout);
			void			_DisallowChar(BTextView* textView);
			void			_SetupTranslatorMenu(BMenu* translatorMenu);
			void			_SetupOutputPathMenu(BMenu* outputPathMenu);
			BString			_FindValidFileName(const char* name) const;
			void			_CenterAndShow();

			void			_TakeScreenshot();
			status_t		_GetActiveWindowFrame(BRect* frame);

			void			_SaveScreenshot();
			void			_SaveScreenshotSilent() const;

private:
			BBox*			fPreviewBox;
			BRadioButton*	fActiveWindow;
			BRadioButton*	fWholeDesktop;
			BTextControl*	fDelayControl;
			BCheckBox*		fWindowBorder;
			BCheckBox*		fShowCursor;
			BButton*		fBackToSave;
			BButton*		fTakeScreenshot;
			BTextControl*	fNameControl;
			BMenu*			fTranslatorMenu;
			BMenu*			fOutputPathMenu;
			BBitmap*		fScreenshot;
			BFilePanel*		fOutputPathPanel;
			BMenuItem*		fLastSelectedPath;

			bigtime_t		fDelay;

			bool			fIncludeBorder;
			bool			fIncludeCursor;
			bool			fGrabActiveWindow;
			bool			fShowConfigWindow;
			bool			fSaveScreenshotSilent;

			int32			fTranslator;
			int32			fImageFileType;
};
