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
		status_t	UnregisterAddOn(BEntry &entry);
		void		RegisterAddOns();

		void		RegisterDevice(BInputServerDevice *isd, const entry_ref &ref, image_id addon_image);
		void		RegisterFilter(BInputServerFilter *isf, const entry_ref &ref, image_id addon_image);
		void		RegisterMethod(BInputServerMethod *ism, const entry_ref &ref, image_id addon_image);
		
	private:
		struct device_info {
			entry_ref ref;
			_BDeviceAddOn_* addon;
			image_id addon_image;
			BInputServerDevice *isd;
		};
		struct filter_info {
			entry_ref ref;
			image_id addon_image;
			BInputServerFilter *isf;
		};
		struct method_info {
			entry_ref ref;
			_BMethodAddOn_* addon;
			image_id addon_image;
			BInputServerMethod *ism;
		};

		BLocker fLock;
		List<device_info> fDeviceList;
		List<filter_info> fFilterList;
		List<method_info> fMethodList;

		AddOnMonitorHandler	*fHandler;
		AddOnMonitor		*fAddOnMonitor;
};

#endif // _ADD_ON_MANAGER_H
