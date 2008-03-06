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
			printf("## fdesc %d\n", fFD);
			if (fFD > 0) {
				// find out which ID was assigned
				status = ioctl(fFD, GET_HCI_ID, &fHID, 0);
				printf("## id fdesc %ld ### %ld\n", fHID, status);									

			}
			else {
				fHID = B_ERROR;
			}
	    
					

		}
						
		hci_id GetID(void)
		{
			return fHID;
		}
				
		virtual status_t IssueCommand(raw_command rc, size_t size)
		{
			return B_ERROR;
		}
	
	protected:
		

		hci_id fHID;
		int fFD;
	
	private:


};

#endif
