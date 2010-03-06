////////////////////////////////////////////////////////////////////////////////
//
//	File: GIFView.h
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

// Additional authors:	Stephan Aßmus, <superstippi@gmx.de>

#ifndef GIFVIEW_H
#define GIFVIEW_H

#include <View.h>

class BMenuField;
class BPopUpMenu;
class BMenuItem;
class BCheckBox;
class BRadioButton;
class BTextControl;
class Prefs;

#define GV_WEB_SAFE					'gvws'
#define GV_BEOS_SYSTEM				'gvbe'
#define GV_GREYSCALE				'gvgr'
#define GV_OPTIMAL					'gvop'
#define GV_INTERLACED				'gvin'
#define GV_USE_DITHERING			'gvud'
#define GV_USE_TRANSPARENT			'gvut'
#define GV_USE_TRANSPARENT_AUTO		'gvua'
#define GV_USE_TRANSPARENT_COLOR	'gvuc'
#define GV_TRANSPARENT_RED			'gvtr'
#define GV_TRANSPARENT_GREEN		'gvtg'
#define GV_TRANSPARENT_BLUE			'gvtb'
#define GV_SET_COLOR_COUNT			'gvcc'

class GIFView : public BView {
 public:
							GIFView(const char* name);
	virtual					~GIFView();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			AllAttached();
		
 private:
			void			RestorePrefs();
			int				CheckInput(BTextControl* control);

		Prefs*				fPrefs;

		BMenuField*			fPaletteMF;
		BPopUpMenu*			fPaletteM;
		BMenuItem*			fWebSafeMI;
		BMenuItem*			fBeOSSystemMI;
		BMenuItem*			fGreyScaleMI;
		BMenuItem*			fOptimalMI;

		BMenuField*			fColorCountMF;
		BPopUpMenu*			fColorCountM;
		BMenuItem*			fColorCountMI[8];
		BMenuItem*			fColorCount256MI;

		BCheckBox*			fInterlacedCB;
		BCheckBox*			fUseTransparentCB;
		BCheckBox*			fUseDitheringCB;

		BRadioButton*		fUseTransparentAutoRB;
		BRadioButton*		fUseTransparentColorRB;

		BTextControl*		fTransparentRedTC;
		BTextControl*		fTransparentGreenTC;
		BTextControl*		fTransparentBlueTC;
};

#endif

