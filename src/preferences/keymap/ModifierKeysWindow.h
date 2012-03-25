/*
 * Copyright 2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef MODIFIER_KEYS_WINDOW_H
#define MODIFIER_KEYS_WINDOW_H


#include <Button.h>
#include <CheckBox.h>
#include <InterfaceDefs.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Window.h>


class ModifierKeysWindow : public BWindow {
public:
							ModifierKeysWindow();
	virtual					~ModifierKeysWindow();

	virtual void			MessageReceived(BMessage* message);

protected:
			BMenuField*		_CreateCapsLockMenuField();
			BMenuField*		_CreateControlMenuField();
			BMenuField*		_CreateOptionMenuField();
			BMenuField*		_CreateCommandMenuField();
                            
			void			_MarkMenuItems();
			const char*		_KeyToString(int32 key);
			uint32			_KeyToKeyCode(int32 key, bool right = false);

private:                    
			BPopUpMenu*		fCapsLockMenu;
			BPopUpMenu*		fControlMenu;
			BPopUpMenu*		fOptionMenu;
			BPopUpMenu*		fCommandMenu;

			BCheckBox*		fSwitchRight;

			BButton*		fRevertButton;
			BButton*		fCancelButton;
			BButton*		fOkButton;
                            
			key_map*		fCurrentMap;
			key_map*		fSavedMap;
                            
			char*			fCurrentBuffer;
			char*			fSavedBuffer;
};


#endif	// MODIFIER_KEYS_WINDOW_H
