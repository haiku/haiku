/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */


#include "LocalDeviceHandler.h"

    
    
LocalDeviceHandler::LocalDeviceHandler(HCIDelegate* hd)
{
	fHCIDelegate = hd;
	fProperties = new BMessage();
	
}

LocalDeviceHandler::~LocalDeviceHandler() 
{

}


hci_id
LocalDeviceHandler::GetID()
{
    return fHCIDelegate->GetID();
}


bool 
LocalDeviceHandler::Available() {

	return true;
}


void
LocalDeviceHandler::Acquire(void) {

}


bool
LocalDeviceHandler::IsPropertyAvailable(const BString& property) {
	
	type_code typeFound;
	int32     countFound;

	return (fProperties->GetInfo(property.String(), &typeFound, &countFound) == B_OK );

}


void
LocalDeviceHandler::AddWantedEvent(BMessage* msg) 
{
    fEventsWanted.Lock();
    // TODO: review why it is neede to replicate the msg
    fEventsWanted.AddMessage(msg);
    fEventsWanted.Unlock();
}


void 
LocalDeviceHandler::ClearWantedEvent(BMessage* msg, uint16 event = 0, uint16 opcode = 0)
{  
    // Remove the whole petition from queue
    fEventsWanted.Lock();        
/*    
    if (event == 0) {
        fEventsWanted.RemoveMessage(msg);
        goto bail;
    }

    int16 eventFound;
    int16 opcodeFound;
	int32 eventIndex;
	   
    for (int32 index = 0 ; index < fEventsWanted.CountMessages() ; index++) {
        
        BMessage* msg = fEventsWanted.FindMessage(index);

       	eventIndex = 0; 
        // for each Event    
        while (msg->FindInt16("eventExpected", eventIndex, &eventFound) == B_OK ) {
            if (eventFound == event) {
                
                // there is an opcode specified
                if (opcde != 0)
                	
                	// The opcode matches
                	if ( (msg->FindInt16("opcodeExpected", eventIndex, &opcodeFound) == B_OK) &&
                	     ((uint16)opcodeFound != opcode) ) {
                	     
                		fEventsWanted.RemoveMessage(msg);
                		goto bail;
                	}                    
                }  else {                
                	// Event matches so far
               		fEventsWanted.RemoveMessage(msg);
               		goto bail;
                }
                
            } 
            eventIndex++;
        }
    }
            
bail:    */
    fEventsWanted.Unlock();
    
    fEventsWanted.RemoveMessage(msg);
}


BMessage*
LocalDeviceHandler::FindPetition(uint16 event, uint16 opcode = 0)
{
    int16 eventFound;
    int16 opcodeFound;
	int32 eventIndex;
	    
    fEventsWanted.Lock();        
    // for each Petition
    for (int32 index = 0 ; index < fEventsWanted.CountMessages() ; index++) {
        BMessage* msg = fEventsWanted.FindMessage(index);
		
		printf("Petition %ld ... of %ld msg #%p\n", index, fEventsWanted.CountMessages(), msg);
		msg->PrintToStream();
       	eventIndex = 0;         
        // for each Event    
        while (msg->FindInt16("eventExpected", eventIndex, &eventFound) == B_OK ) {
            if (eventFound == event) {

        		printf("Event found %ld\n", eventIndex);        
                // there is an opcode specified.. 
                if (msg->FindInt16("opcodeExpected", eventIndex, &opcodeFound) == B_OK) {
                	
                	// ensure the opcode
                	if ((uint16)opcodeFound != opcode) {
		        		printf("opcode does not match %d\n", opcode);        
    	                break;
    	            }
              		printf("Opcdodes match %d %d \n", opcode , opcodeFound);        
                } 
                                    
	            fEventsWanted.Unlock();
    	        return msg;
    	        

            } 
            eventIndex++;
        }
    }
    
    fEventsWanted.Unlock();
    return NULL;
}
