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


#include <Window.h>


class BButton;
class BMenuField;
class BPopUpMenu;
class StatusMenuField;


class ModifierKeysWindow : public BWindow {
public:
									ModifierKeysWindow();
	virtual							~ModifierKeysWindow();

	virtual	void					MessageReceived(BMessage* message);

private:
			void					_CreateMenuField(BPopUpMenu** _menu, BMenuField** _field,
										uint32 key, const char* label);
			void					_MarkMenuItems();
			bool					_MarkMenuItem(const char*, BPopUpMenu*, uint32 l, uint32 r);
			const char*				_KeyToString(int32 key);
			int32					_KeyToKeyCode(int32 key, bool right = false);
			int32					_LastKey();
			void					_ValidateDuplicateKeys();
			void					_ValidateDuplicateKey(StatusMenuField*, uint32);
			uint32					_DuplicateKeys();
			void					_UpdateStatus();

			BPopUpMenu*				fCapsMenu;
			BPopUpMenu*				fShiftMenu;
			BPopUpMenu*				fControlMenu;
			BPopUpMenu*				fOptionMenu;
			BPopUpMenu*				fCommandMenu;

			StatusMenuField*		fCapsField;
			StatusMenuField*		fShiftField;
			StatusMenuField*		fControlField;
			StatusMenuField*		fOptionField;
			StatusMenuField*		fCommandField;

			BButton*				fRevertButton;
			BButton*				fCancelButton;
			BButton*				fOkButton;

			key_map*				fCurrentMap;
			key_map*				fSavedMap;

			char*					fCurrentBuffer;
			char*					fSavedBuffer;
};


#endif	// MODIFIER_KEYS_WINDOW_H
