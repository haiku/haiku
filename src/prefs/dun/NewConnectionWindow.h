/*

NewConnectionWindow Header

Author: Sikosis (beos@gravity24hr.com)

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __NEWCONNECTIONWINDOW_H__
#define __NEWCONNECTIONWINDOW_H__

#include <Application.h>
#include <Alert.h>
#include <Button.h>
#include <Screen.h>
#include <stdio.h>
#include <TextControl.h>
#include <TextView.h>
#include <Window.h>
#include <View.h>

#include "DUNView.h"

class NewConnectionWindowView; 

class NewConnectionWindow : public BWindow
{
	public:
    	NewConnectionWindow(BRect frame);
	    ~NewConnectionWindow();
	    virtual void MessageReceived(BMessage *message);
	private:
		void InitWindow(void);
		
		NewConnectionWindowView*	ptrNewConnectionWindowView;
	    BButton         		    *btnAdd;
	    BButton         		    *btnCancel;
	    BTextControl                *txtNewConnection;
	    
};

#endif
