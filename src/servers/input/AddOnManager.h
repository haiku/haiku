/*
** Copyright 2004, the OpenBeOS project. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Author : Jérôme Duval
** Original authors: Marcus Overhagen, Axel Dörfler
*/
#ifndef _ADD_ON_MANAGER_H
#define _ADD_ON_MANAGER_H

// Manager for input_server add-ons (devices, filters, methods)

#include <Locker.h>
#include <InputServerDevice.h>
#include <InputServerFilter.h>
#include <InputServerMethod.h>
#include "AddOnMonitor.h"
#include "AddOnMonitorHandler.h"
#include "TList.h"

class AddOnManager {
	public:
		AddOnManager();
		~AddOnManager();

		void		LoadState();
		void		SaveState();

	private:
		status_t	RegisterAddOn(BEntry &entry);
		void		RegisterAddOns();

		void		RegisterDevice(BInputServerDevice *isd, const entry_ref &ref);
		void		RegisterFilter(BInputServerFilter *isf, const entry_ref &ref);
		void		RegisterMethod(BInputServerMethod *ism, const entry_ref &ref);
		
	private:
		struct device_info {
			entry_ref ref;
		};
		struct filter_info {
			entry_ref ref;
		};
		struct method_info {
			entry_ref ref;
		};

		BLocker fLock;
		List<device_info> fDeviceList;
		List<filter_info> fFilterList;
		List<method_info> fMethodList;

		AddOnMonitorHandler	*fHandler;
		AddOnMonitor		*fAddOnMonitor;
};

#endif // _ADD_ON_MANAGER_H
