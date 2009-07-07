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
			//TODO create such queue
			fIdentifier = -1;
		}


		hci_id Id(void) const
		{
			return fIdentifier;
		}


		virtual ~HCIDelegate()
 		{

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
			// are actually send to HW to implement HCI FlowControl requeriments
			return IssueCommand(rc, size);
		}

	protected:

		hci_id fIdentifier;

	private:

};

#endif
