/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef AUTO_CONFIG_WINDOW_H
#define AUTO_CONFIG_WINDOW_H


#include <Box.h>
#include <Button.h>
#include <View.h>
#include <Window.h>

#include "MailSettings.h"

#include "AutoConfigView.h"
#include "ConfigWindow.h"


//message constants
const int32	kBackMsg	=	'?bac';
const int32	kOkMsg		=	'?bok';

class AutoConfigWindow : public BWindow
{
public:
							AutoConfigWindow(BRect rect, ConfigWindow *parent);
							~AutoConfigWindow();
		virtual void		MessageReceived(BMessage *msg);
		virtual bool		QuitRequested(void);
						  
private:
		account_info 		fAccountInfo;
		
		BMailAccountSettings*	GenerateBasicAccount();
		
		BView				*fRootView;
		BRect				fBoxRect;
		ConfigWindow		*fParentWindow;
		BMailAccountSettings	*fAccount;
		AutoConfigView		*fMainView;
		ServerSettingsView	*fServerView;
		BButton				*fBackButton;
		BButton				*fNextButton;
		
		bool				fMainConfigState;
		bool				fServerConfigState;
		bool				fAutoConfigServer;
		
		AutoConfig			fAutoConfig;
};



#endif