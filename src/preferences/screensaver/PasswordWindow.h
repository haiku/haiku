/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PASSWORDWINDOW_H
#define PASSWORDWINDOW_H

#include "Window.h"
#include "CheckBox.h"
#include "String.h"
#include "Box.h"
#include "TextControl.h"
#include "Button.h"
#include "Constants.h"

class PasswordWindow : public BWindow
{
	public:
		PasswordWindow();
		void Setup();
		void Update();
		virtual void MessageReceived(BMessage *message);

	private:
		BRadioButton *fUseNetwork,*fUseCustom;
		BBox *fCustomBox;
		BTextControl *fPassword,*fConfirm;
		BButton *fCancel,*fDone;
		BString fThePassword; 
		bool fUseNetPassword;

};

#endif // PASSWORDWINDOW_H

