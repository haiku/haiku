/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _LOCALDEVICE_HANDLER_H_
#define _LOCALDEVICE_HANDLER_H_

#include <String.h>

#include <MessageQueue.h>

#include <bluetooth/bluetooth.h>

#include "HCIDelegate.h"

class LocalDeviceHandler {

public:

	virtual hci_id GetID();
	
	virtual bool Available();	
	void Acquire(void);
	
	BMessage* 	  GetPropertiesMessage(void) { return fProperties; }
	bool  		  IsPropertyAvailable(const BString& property);
	

protected:    
	LocalDeviceHandler (HCIDelegate* hd);
	virtual ~LocalDeviceHandler();

	HCIDelegate*	fHCIDelegate;    
	BMessage* 		fProperties;
	
    void 		AddWantedEvent(BMessage* msg);    
    void 		ClearWantedEvent(BMessage* msg, uint16 event, uint16 opcode = 0);
    void 		ClearWantedEvent(BMessage* msg);
        
    BMessage* 	FindPetition(uint16 event, uint16 opcode = 0);

private:    
    
	BMessageQueue   fEventsWanted;

	
};

#endif
