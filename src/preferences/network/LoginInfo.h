#ifndef LOGIN_INFO_H
#define LOGIN_INFO_H

#include <Window.h>
#include <Button.h>
#include <TextControl.h>
class NetworkWindow;

// This is the window that appears when you press the 'Login Info' button
class LoginInfo : public BWindow
{
public:
	LoginInfo(void);
	
	BView   		*fView;
	BTextControl 	*fName;
	BTextControl 	*fPassword;
	BTextControl 	*fConfirm;
	BButton 		*fCancel;
	BButton			*fDone;	
	
	NetworkWindow	*fParentWindow;
	
	virtual void MessageReceived(BMessage *message);	
	
	static const int32 kLogin_Info_Cancel_M  = 'licm';
	static const int32 kLogin_Info_Done_M	 = 'lidn';	
};

#endif
