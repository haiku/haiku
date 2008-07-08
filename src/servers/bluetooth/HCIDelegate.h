/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
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
				printf("%s: hid retrieved %ld status=%ld\n", __FUNCTION__, fHID, status);
			}
			else {
				printf("%s: Device driver could not be opened %ld\n", __FUNCTION__, fHID);
				fHID = B_ERROR;
			}
	    
					

		}
						
		
		hci_id GetID(void)
		{
			return fHID;
		}

		virtual status_t IssueCommand(raw_command rc, size_t size)=0;		
		virtual status_t Launch()=0;

	protected:
		

		hci_id fHID;
		int fFD;
	
	private:


};

#endif
