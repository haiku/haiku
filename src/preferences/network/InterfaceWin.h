#ifndef INTERFACE_WIN_H
#define INTERFACE_WIN_H

#include <Window.h>
#include <RadioButton.h>
#include <Button.h>
#include <Box.h>
#include <TextControl.h>

#include "ConfigData.h"
#include "NetworkWindow.h"

// This is the window that appears when you press the 'Settings' button.
class InterfaceWin : public BWindow 
{                                   
public:
	InterfaceWin(const BRect &frame, const InterfaceData &data);
	
	BStringView		*fName;
	BCheckBox		*fEnabled;
	BRadioButton 	*fDHCP;
	BRadioButton	*fManual;
	BButton 		*fConfig;
	BButton 		*fCancel;
	BButton			*fDone;			
	BBox			*fBox;
	BTextControl 	*fIPAddress;
	BTextControl 	*fNetMask;
	BTextControl 	*fGateway;									
	
	NetworkWindow *fParentWindow;
	
	InterfaceData 	fData;
	bool fChanged;
	
	virtual void MessageReceived(BMessage *message);
	
	static const int32 kCancel_inter_M		  = 'caim' ;		
	static const int32 kDone_inter_M		  = 'doim' ;
	static const int32 kSOMETHING_HAS_CHANGED = 'sohc' ;					
};

#endif
