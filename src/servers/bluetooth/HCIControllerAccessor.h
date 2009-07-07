/*
 * Copyright 2007-2008 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _HCICONTROLLER_ACCESSOR_H_
#define _HCICONTROLLER_ACCESSOR_H_

#include "HCIDelegate.h"


class HCIControllerAccessor : public HCIDelegate {

	public:
		HCIControllerAccessor(BPath* path);
		~HCIControllerAccessor();
		status_t IssueCommand(raw_command rc,  size_t size);
		status_t Launch();
	private:
		int fSocket;
		
};

#endif
