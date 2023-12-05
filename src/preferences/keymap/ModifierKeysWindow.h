/*
 * Copyright 2011-2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Jorge Acereda, jacereda@gmail.com
 */
#ifndef MODIFIER_KEYS_WINDOW_H
#define MODIFIER_KEYS_WINDOW_H


#include <View.h>
#include <Window.h>


class BMenuField;
class BPopUpMenu;


class ConflictView : public BView {
public:
								ConflictView(const char* name);
								~ConflictView();

	virtual	void				Draw(BRect updateRect);

			BBitmap*			Icon();
			void				SetStopIcon(bool show);
			void				SetWarnIcon(bool show);

private:
			void				_FillIcons();

			BBitmap*			fIcon;
			BBitmap*			fStopIcon;
			BBitmap*			fWarnIcon;
};


class ModifierKeysWindow : public BWindow {
public:
									ModifierKeysWindow();
	virtual							~ModifierKeysWindow();

	virtual	void					MessageReceived(BMessage* message);

private:
			void					_CreateMenuField(BPopUpMenu** _menu, BMenuField** _field,
										uint32 key, const char* label);
			void					_MarkMenuItems();
			void					_MarkMenuItem(BPopUpMenu* menu, ConflictView* conflictView,
										uint32 leftKey, uint32 rightKey);
			const char*				_KeyToString(int32 key);
			uint32					_KeyToKeyCode(int32 key,
										bool right = false);
			int32					_LastKey();
			void					_ValidateDuplicateKeys();
			void					_ValidateDuplicateKey(ConflictView* view, uint32 mask);
			uint32					_DuplicateKeys();
			void					_HideShowStatusIcons();
			void					_ToggleStatusIcon(ConflictView* view);
			void					_UpdateStatus();

			BPopUpMenu*				fCapsMenu;
			BPopUpMenu*				fShiftMenu;
			BPopUpMenu*				fControlMenu;
			BPopUpMenu*				fOptionMenu;
			BPopUpMenu*				fCommandMenu;

			ConflictView*			fCapsConflictView;
			ConflictView*			fShiftConflictView;
			ConflictView*			fControlConflictView;
			ConflictView*			fOptionConflictView;
			ConflictView*			fCommandConflictView;

			BButton*				fRevertButton;
			BButton*				fCancelButton;
			BButton*				fOkButton;

			key_map*				fCurrentMap;
			key_map*				fSavedMap;

			char*					fCurrentBuffer;
			char*					fSavedBuffer;
};


#endif	// MODIFIER_KEYS_WINDOW_H
