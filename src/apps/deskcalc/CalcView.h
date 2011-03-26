/*
 * Copyright 2006 Haiku, Inc. All Rights Reserved.
 * Copyright 1997, 1998 R3 Software Ltd. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Timothy Wayper <timmy@wunderbear.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef _CALC_VIEW_H
#define _CALC_VIEW_H

#include <View.h>

class BString;
class BMenuItem;
class BPopUpMenu;
class CalcOptions;
class CalcOptionsWindow;
class ExpressionTextView;

_EXPORT
class CalcView : public BView {
 public:

	static	CalcView*			Instantiate(BMessage* archive);


								CalcView(BRect frame,
									rgb_color rgbBaseColor,
									BMessage* settings);

								CalcView(BMessage* archive);

	virtual						~CalcView();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				FrameResized(float width, float height);

			// Present about box for view (replicant).
	virtual	void				AboutRequested();

			// Archive this view.
	virtual	status_t			Archive(BMessage* archive, bool deep) const;

			// Cut contents of view to system clipboard.
			void				Cut();

			// Copy contents of view to system clipboard.
			void				Copy();

			// Paste contents of system clipboard to view.
			void				Paste(BMessage* message);

			// Save current settings
			status_t			SaveSettings(BMessage* archive) const;

			void				Evaluate();

			void				FlashKey(const char* bytes, int32 numBytes);

 private:
			void				_ParseCalcDesc(const char* keypadDescription);

			void				_PressKey(int key);
			void				_PressKey(const char* label);
			int32				_KeyForLabel(const char* label) const;
			void				_FlashKey(int32 key, uint32 flashFlags);
			void				_AudioFeedback(bool inBackGround);

			void				_Colorize();

			void				_CreatePopUpMenu();

			BRect				_ExpressionRect() const;
			BRect				_KeypadRect() const;

			void				_ShowKeypad(bool show);
			void				_FetchAppIcon(BBitmap* into);

			status_t			_LoadSettings(BMessage* archive);

			// grid dimensions
			int16				fColums;
			int16				fRows;

			// color scheme
			rgb_color			fBaseColor;
			rgb_color			fLightColor;
			rgb_color			fDarkColor;
			rgb_color			fButtonTextColor;
			rgb_color			fExpressionBGColor;
			rgb_color			fExpressionTextColor;

			// view dimensions
			float				fWidth;
			float				fHeight;

			// keypad grid
			struct CalcKey;

			char*				fKeypadDescription;
			CalcKey*			fKeypad;

			// icon
			BBitmap*			fCalcIcon;

			// expression
			ExpressionTextView*	fExpressionTextView;

			// pop-up context menu.
			BPopUpMenu*			fPopUpMenu;
			BMenuItem*			fAutoNumlockItem;
			BMenuItem*			fAudioFeedbackItem;
			BMenuItem*			fShowKeypadItem;

			// calculator options.
			CalcOptions*		fOptions;
			bool				fShowKeypad;
};

#endif // _CALC_VIEW_H
