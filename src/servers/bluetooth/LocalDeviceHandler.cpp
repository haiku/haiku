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
    // TODO: review why it is needed to replicate the msg
    printf("Adding request... %p\n", msg);
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

		printf("%s:Event expected@%ld...\n", __FUNCTION__, eventIndex);

		if (eventFound == event) {

			printf("%s:Event matches@%ld", __FUNCTION__, eventIndex);
			// there is an opcode specified
			if (opcode != 0) {

				// The opcode matches
				if ( (msg->FindInt16("opcodeExpected", eventIndex, &opcodeFound) == B_OK) &&
					((uint16)opcodeFound == opcode) ) {

					// this should remove only the entry
					printf("Removed event %#x and opcode %d from request %p\n", event, opcode, msg); 
					(void)msg->RemoveData("eventExpected", eventIndex);
					(void)msg->RemoveData("opcodeExpected", eventIndex);
					goto finish;
				}

			}  else {
				// Event matches so far
				printf("Removed event %d from message %p\n", event, msg); 
				(void)msg->RemoveData("eventExpected", eventIndex);
				goto finish;
			}

		}
		eventIndex++;
	}
	printf("%s:Nothing Found/Removed\n",__FUNCTION__); 

finish:
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
		
		printf("%s:Petition %ld ... of %ld msg #%p\n", __FUNCTION__, index, fEventsWanted.CountMessages(), msg);
		//msg->PrintToStream();
		eventIndex = 0;

        // for each Event
        while (msg->FindInt16("eventExpected", eventIndex, &eventFound) == B_OK ) {
            if (eventFound == event) {

        		printf("%s:Event found@%ld...", __FUNCTION__, eventIndex);
                // there is an opcode specified.. 
                if (msg->FindInt16("opcodeExpected", eventIndex, &opcodeFound) == B_OK) {

                	// ensure the opcode
                	if ((uint16)opcodeFound != opcode) {
		        		printf("%s:opcode does not match %d\n",__FUNCTION__, opcode);
    	                break;
    	            }
					printf("opcode match %d\n", opcode);
                } else {
                	printf("no opcode specified\n");
                }

	            fEventsWanted.Unlock();
				if (indexFound != NULL)
					*indexFound = eventIndex;
    	        return msg;


            } 
            eventIndex++;
        }
    }
  	printf("%s:Nothing found\n",__FUNCTION__);

    fEventsWanted.Unlock();
    return NULL;
}
