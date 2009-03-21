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

	hci_id GetID();
	
	bool Available();	
	void Acquire(void);
	status_t Launch(void);
	
	BMessage* 	  GetPropertiesMessage(void) { return fProperties; }
	bool  		  IsPropertyAvailable(const char* property);
	

protected:    
	LocalDeviceHandler (HCIDelegate* hd);
	virtual ~LocalDeviceHandler();

	HCIDelegate*	fHCIDelegate;    
	BMessage* 		fProperties;
	
    void 		AddWantedEvent(BMessage* msg);    
    void 		ClearWantedEvent(BMessage* msg, uint16 event, uint16 opcode = 0);
    void 		ClearWantedEvent(BMessage* msg);
        
    BMessage* 	FindPetition(uint16 event, uint16 opcode = 0, int32* indexFound = NULL);

private:    
    
	BMessageQueue   fEventsWanted;

	
};

#endif
