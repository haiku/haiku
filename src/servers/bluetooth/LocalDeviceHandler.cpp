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

status_t
LocalDeviceHandler::Launch(void)
{
    return fHCIDelegate->Launch();
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
LocalDeviceHandler::ClearWantedEvent(BMessage* msg)
{
    fEventsWanted.Lock();
    fEventsWanted.RemoveMessage(msg);    
    fEventsWanted.Unlock();

}


void 
LocalDeviceHandler::ClearWantedEvent(BMessage* msg, uint16 event, uint16 opcode)
{  
    // Remove the whole petition from queue
    fEventsWanted.Lock();

    int16 eventFound;
    int16 opcodeFound;
	int32 eventIndex = 0;

    // for each Event    
	while (msg->FindInt16("eventExpected", eventIndex, &eventFound) == B_OK ) {

		printf("@ Event expected found @%ld\n", eventIndex);

		if (eventFound == event) {

			printf("@ Event matches %ld\n", eventIndex);
			// there is an opcode specified
			if (opcode != 0) {

				// The opcode matches
				if ( (msg->FindInt16("opcodeExpected", eventIndex, &opcodeFound) == B_OK) &&
					((uint16)opcodeFound == opcode) ) {

					// this should remove only the entry
					printf("@ Removed event %d and opcode %d from message %p\n", event, opcode, msg); 
					(void)msg->RemoveData("eventExpected", eventIndex);
					(void)msg->RemoveData("opcodeExpected", eventIndex);
					goto bail;
				}
			}  else {
				// Event matches so far
				printf("@ Removed event %d from message %p\n", event, msg); 
				(void)msg->RemoveData("eventExpected", eventIndex);
				goto bail;
			}

		}
		eventIndex++;
	}

bail:

	fEventsWanted.Unlock();

}


BMessage*
LocalDeviceHandler::FindPetition(uint16 event, uint16 opcode, int32* indexFound)
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
              		printf("Opcodes match %d %d \n", opcode , opcodeFound);
                } 

	            fEventsWanted.Unlock();
				if (indexFound != NULL)
					*indexFound = eventIndex;
    	        return msg;


            } 
            eventIndex++;
        }
    }

    fEventsWanted.Unlock();
    return NULL;
}
