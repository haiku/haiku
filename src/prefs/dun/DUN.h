/*

DUN Header - DialUp Networking Header

Authors: Sikosis (beos@gravity24hr.com)
		 Misza (misza@ihug.com.au) 

(C) 2002-2003 OpenBeOS under MIT license

*/

#ifndef __DUN_H__
#define __DUN_H__

#include "DUNWindow.h"
#include <iostream>

extern const char *APP_SIGNATURE;

class DUN : public BApplication {
public:
   DUN();
   virtual void MessageReceived(BMessage *message);
private:
	DUNWindow*	 ptrDUNWindow;
};

#endif
