/*

DetailsView Header 

Author: Misza (misza@ihug.com.au)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __DETAILSVIEW_H__
#define __DETAILSVIEW_H__

#include <InterfaceKit.h>
#include <iostream>
#include <Application.h>

const uint32 CHANGE_PHONENO = 'Cphn';
const uint32 CHANGE_USERNAME = 'Cusr';
const uint32 CHANGE_PASSWORD = 'Cpwd';
const uint32 SAVEPASSWORD = 'Spwd';
const uint32 BTN_SETTINGS = 'Stng';

class DetailsView: public BView
{	

	public:
		DetailsView();
		virtual void MessageReceived(BMessage* msg);
		virtual void AttachedToWindow();
	private:
		BTextControl* phoneno;
		BTextControl* username;
		BTextControl* passwd;
		
		BCheckBox* savepasswd;
		
		BButton* settingsbtn;
};

#endif
