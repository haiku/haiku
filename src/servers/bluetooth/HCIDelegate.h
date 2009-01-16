/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _HCIDELEGATE_H_
#define _HCIDELEGATE_H_

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <Path.h>

#include <bluetooth/HCI/btHCI_transport.h>


typedef void* raw_command;


class HCIDelegate {

	public:
		HCIDelegate(BPath* path)
		{
			status_t status;
				
			fFD = open (path->Path(), O_RDWR);
			if (fFD > 0) {
				// find out which ID was assigned
				status = ioctl(fFD, GET_HCI_ID, &fHID, 0);
				printf("%s: hid retrieved %lx status=%ld\n", __FUNCTION__, 
					fHID, status);
			}
			else {
				printf("%s: Device driver could not be opened %ld\n", __FUNCTION__, 
					fHID);
				fHID = B_ERROR;
			}

			//TODO create such queue

		}

		
		hci_id GetID(void)
		{
			return fHID;
		}


		virtual ~HCIDelegate()
 		{
			if (fFD  > 0)
			{
				close (fFD);
				fFD = -1;
				fHID = B_ERROR;
			}
		}

		virtual status_t IssueCommand(raw_command rc, size_t size)=0; 
			// TODO means to be private use QueueCommand
		virtual status_t Launch()=0;


		void FreeWindow(uint8 slots)
		{
			// TODO: hci control flow
		} 


		status_t QueueCommand(raw_command rc, size_t size) 
		{
			// TODO: this is suposed to queue the command in a queue so all
			//  are actually send to HW
			return IssueCommand(rc, size);
		}

	protected:

		hci_id fHID;
		int fFD;

	private:


};

#endif
