/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#include "BPortNot.h"
#include "Output.h"

#include "BluetoothServer.h"
#include "LocalDeviceImpl.h"

#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_transport.h>

#include <stdio.h>

BPortNot::BPortNot(BluetoothServer* app, const char* name) {

	ourapp = (BluetoothServer*) app;		

	fPort = find_port(name);	
	if ( fPort == B_NAME_NOT_FOUND ) 
	{
		fPort = create_port(256, name);
		Output::Instance()->Post("Event Listener Port Created\n",BLACKBOARD_GENERAL);
	}
}

void
BPortNot::loop()
{
	size_t 		size;
	size_t 		ssize;
	int32		code;
	
	struct hci_event_header *hdr;
	
	LocalDeviceImpl* ld;

	while (code != HCI_HAIKU_EVENT_SERVER_QUITTING) { 

		 size = port_buffer_size(fPort);
		          
         /* TODO: Consider using snb_buff like structure here or 
            max event size to prevent mallocs & frees */
	     hdr = (struct hci_event_header *) malloc(size);	     
		 ssize = read_port(fPort, &code, hdr, size);		 		 

		 if (size != ssize) {
		    Output::Instance()->Post("Event size not matching", BLACKBOARD_GENERAL);
		    continue;
		 }

		 if (size <= 0) {
		    Output::Instance()->Post("Suspicious empty event", BLACKBOARD_GENERAL);
		    continue;
		 }

		 // we only handle events
		 if (GET_PORTCODE_TYPE(code)!= BT_EVENT) {
		    Output::Instance()->Post("Wrong type frame code", BLACKBOARD_GENERAL);
		    continue;
		 }
		 	    
         ld = ourapp->LocateLocalDeviceImpl(GET_PORTCODE_HID(code));
		 if (ld == NULL) {
		    Output::Instance()->Post("LocalDevice could not be fetched", BLACKBOARD_EVENTS);
		    continue;
		 }


		 ld->HandleEvent(hdr);


		 // free the event
		 free(hdr);
	}	

}
