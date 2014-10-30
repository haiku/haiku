/*
 * Copyright 2004-2011 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "NetworkSetupAddOn.h"

#include <GroupView.h>


class BListView;


class ServicesAddOn : public NetworkSetupAddOn, public BGroupView {
	public:
							ServicesAddOn(image_id addon_image);
		BView*				CreateView();

		void				AttachedToWindow();
		void				MessageReceived(BMessage*);

		status_t			Save();
		status_t			Revert();

	private:
		status_t			_ParseInetd();
		status_t			_ParseXinetd();

	private:
		BListView*			fServicesListView;
};
