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

enum {
	MSG_OPTIONS_AUTO_NUM_LOCK				= 'oanl',
	MSG_OPTIONS_AUDIO_FEEDBACK				= 'oafb',
	MSG_OPTIONS_ANGLE_MODE					= 'oamd',
	MSG_OPTIONS_KEYPAD_MODE_COMPACT			= 'okmc',
	MSG_OPTIONS_KEYPAD_MODE_BASIC			= 'okmb',
	MSG_OPTIONS_KEYPAD_MODE_SCIENTIFIC		= 'okms',
	MSG_UNFLASH_KEY							= 'uflk'
};

static const float kMinimumWidthBasic		= 130.0f;
static const float kMaximumWidthBasic		= 400.0f;
static const float kMinimumHeightBasic		= 130.0f;
static const float kMaximumHeightBasic		= 400.0f;

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
	virtual void				ResizeTo(float width, float height);
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

			// Evaluate the expression
			void				Evaluate();

			// Flash the key on the keypad
			void				FlashKey(const char* bytes, int32 numBytes);

			// Toggle whether or not the Num Lock key starts on
			void				ToggleAutoNumlock(void);

			// Toggle whether or not to provide audio feedback
			// (option currently disabled)
			void				ToggleAudioFeedback(void);

			// Toggle radian/degree mode
			void				ToggleAngleMode(void);

			// Set the keypad mode
			void				SetKeypadMode(uint8 mode);

 private:
			void				_ParseCalcDesc(const char* keypadDescription);

			void				_PressKey(int key);
			void				_PressKey(const char* label);
			int32				_KeyForLabel(const char* label) const;
			void				_FlashKey(int32 key, uint32 flashFlags);
			void				_AudioFeedback(bool inBackGround);

			void				_Colorize();

			void				_CreatePopUpMenu(bool addKeypadModeMenuItems);

			BRect				_ExpressionRect() const;
			BRect				_KeypadRect() const;

			void				_MarkKeypadItems(uint8 mode);

			void				_FetchAppIcon(BBitmap* into);

			status_t			_LoadSettings(BMessage* archive);

			// grid dimensions
			int16				fColumns;
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

			BMenuItem*			fAngleModeRadianItem;
			BMenuItem*			fAngleModeDegreeItem;

			BMenuItem*			fKeypadModeCompactItem;
			BMenuItem*			fKeypadModeBasicItem;
			BMenuItem*			fKeypadModeScientificItem;

			// calculator options.
			CalcOptions*		fOptions;
};

#endif // _CALC_VIEW_H
