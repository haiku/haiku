#ifndef AUTO_CONFIG_WINDOW_H
#define AUTO_CONFIG_WINDOW_H

#include "Account.h"
#include "AutoConfigView.h"

#include <View.h>
#include <Box.h>
#include <Button.h>
#include <Window.h>


//message constants
const int32	kBackMsg	=	'?bac';
const int32	kOkMsg		=	'?bok';

class AutoConfigWindow : public BWindow
{
	public:
							AutoConfigWindow(BRect rect, BWindow *parent);
							~AutoConfigWindow();
		virtual void		MessageReceived(BMessage *msg);
		virtual bool		QuitRequested(void);
						  
	private:
		account_info 		fAccountInfo;
		
		Account*			GenerateBasicAccount();
		
		BView				*fRootView;
		BRect				fBoxRect;
		BWindow				*fParentWindow;
		Account				*fAccount;
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