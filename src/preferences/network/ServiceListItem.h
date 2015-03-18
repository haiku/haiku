/*
 * Copyright 2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, <axeld@pinc-software.de>
 */
#ifndef SERVICE_LIST_ITEM_H
#define SERVICE_LIST_ITEM_H


#include <ListItem.h>
#include <NetworkSettings.h>
#include <NetworkSettingsAddOn.h>


using namespace BNetworkKit;


class ServiceListItem : public BListItem,
	public BNetworkKit::BNetworkSettingsListener {
public:
								ServiceListItem(const char* name,
									const char* label,
									const BNetworkSettings& settings);
	virtual						~ServiceListItem();

			const char*			Label() const { return fLabel; }

	virtual	void				DrawItem(BView* owner,
									BRect bounds, bool complete);
	virtual	void				Update(BView* owner, const BFont* font);

	inline	const char*			Name() const { return fName; }

	virtual	void				SettingsUpdated(uint32 type);

protected:
	virtual	bool				IsEnabled();

private:
			const char*			fName;
			const char*			fLabel;
			const BNetworkSettings&
								fSettings;

			BView*				fOwner;
			float				fLineOffset;
			bool				fEnabled;
};


#endif // SERVICE_LIST_ITEM_H
