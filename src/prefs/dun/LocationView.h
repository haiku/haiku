/*

LocationView Header

Authors: Misza (misza@ihug.com.au) 

(C) 2002 OpenBeOS under MIT license

*/

#ifndef __LOCATIONVIEW_H__
#define __LOCATIONVIEW_H__

#include <InterfaceKit.h>
#include <iostream>
#include <Application.h>

const uint32 CHANGE_DCW = 'Cdcw';
const uint32 CHANGE_DOP = 'Cdop';

const uint32 DCW_CHKBOX = 'Dcwc';
const uint32 DOP_CHKBOX = 'Dopc';

const uint32 DCW_ITEMS = 'Dcwi';
const uint32 DOP_ITEMS = 'Dopi';

class LocationView: public BView
{	 
	public:
		LocationView();
        virtual void MessageReceived(BMessage *msg);
		virtual void AttachedToWindow();
	private:
		BCheckBox *disablecallwaiting;
    	BTextControl* dcwvalue;
    	BPopUpMenu *dcwlist;
    	BMenuField* dcwlistfield;
    	BMenuItem* dcwone;
    	BMenuItem* dcwtwo;
    	BMenuItem* dcwthree;
    	
    	BCheckBox *dialoutprefix;
		BTextControl* dopvalue;
		BPopUpMenu *doplist;
		BMenuField* doplistfield;
		BMenuItem* dopone;
    	BMenuItem* doptwo;
    	BMenuItem* dopthree;
};

#endif
