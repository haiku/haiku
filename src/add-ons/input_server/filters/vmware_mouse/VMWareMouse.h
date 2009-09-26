/*
 * Copyright 2009, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef VMWARE_MOUSE_H
#define VMWARE_MOUSE_H

#include <InputServerFilter.h>

class VMWareMouseFilter : public BInputServerFilter {
public:
							VMWareMouseFilter();
virtual						~VMWareMouseFilter();

virtual	status_t			InitCheck();
virtual	filter_result		Filter(BMessage *msg, BList *outList);

private:
		void				_ExecuteCommand(union packet_u &packet);

		bool				_Enable();
		void				_Disable();

		void				_GetStatus(uint16 *status, uint16 *numWords);
		void				_GetPosition(int32 &x, int32 &y);
		void				_ScalePosition(int32 &x, int32 &y);

		bool				fIsEnabled;
};

#endif // VMWARE_MOUSE_H
