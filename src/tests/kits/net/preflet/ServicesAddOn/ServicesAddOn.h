/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "NetworkSetupAddOn.h"

class ServicesAddOn : public NetworkSetupAddOn {
	protected:
		BListView*			fServicesListView;
	public:
							ServicesAddOn(image_id addon_image);
		BView*				CreateView(BRect* bounds);
		const char*			Name() { return "Services"; };
		status_t			ParseInetd();
		status_t			ParseXinetd();
};
