/*
 * Copyright 2008-2026 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 *		Leo Rouleau
 */
#ifndef DEVICESVIEW_H
#define DEVICESVIEW_H


#include <MenuField.h>
#include <MenuItem.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <TabView.h>
#include <View.h>

#include <map>

#include "Device.h"
#include "DeviceACPI.h"
#include "DevicePCI.h"
#include "DeviceSCSI.h"
#include "DeviceUSB.h"
#include "PropertyList.h"

class BButton;
class BStringView;
class BMenuBar;

static const uint32 kMsgRefresh				= 'refr';
static const uint32 kMsgReportCompatibility	= 'repo';
static const uint32 kMsgGenerateSysInfo		= 'sysi';
static const uint32 kMsgSelectionChanged	= 'selc';
static const uint32 kMsgOrderBus			= 'obus';
static const uint32 kMsgOrderCategory		= 'ocat';
static const uint32 kMsgOrderConnection		= 'ocon';
static const uint32 kMsgToggleDriver		= 'disd';
static const uint32 kMsgReboot				= 'rebt';

typedef enum {
	ORDER_BY_BUS,
	ORDER_BY_CATEGORY,
	ORDER_BY_CONNECTION
} OrderByType;

typedef std::map<Category, Device*> CategoryMap;
typedef std::map<Category, Device*>::const_iterator CategoryMapIterator;

typedef std::vector<Device*> Devices;


class DevicesView : public BView {
	public:
				DevicesView(OrderByType OrderBy = ORDER_BY_CATEGORY);
				~DevicesView();

		OrderByType OrderBy() const { return fOrderBy; };

		virtual void CreateLayout();

		virtual void MessageReceived(BMessage* msg);
		virtual void RescanDevices();
		static void CreateCategoryMap(const Devices& devices, CategoryMap& categoryMap);
		static void DeleteCategoryMap(CategoryMap& categoryMap);

		virtual void DeleteDevices();
		static void RebuildDevicesOutline(BOutlineListView* outline, const Devices& devices,
			const CategoryMap& categoryMap, OrderByType orderBy);
		static void AddChildrenToOutlineByConnection(BOutlineListView* outline, const Devices& devices, Device* parent);
		virtual void AddDeviceAndChildren(device_node_cookie* node, Device* parent);
		static int   SortItemsCompare(const BListItem*, const BListItem*);

	private:
		void				_ToggleDriverState(bool disable);
		void				_ShowInfoAlert(const BString& message);
		void				_ShowDisableDriverAlert(const BPath& settingsPath,
								const BString& relativePath);

		void				_SetOrderBy(OrderByType orderBy);

		void				_UpdateBlockButton(Device* device);

		BOutlineListView*	fDevicesOutline;
		PropertyList*		fAttributesView;
		BMenuField*			fOrderByMenu;
		Devices				fDevices;
		OrderByType			fOrderBy;
		CategoryMap			fCategoryMap;
		BButton*			fBlockButton;
		BStringView*		fRebootNotice;
		BMenuBar*			fActionMenuBar;
		bool				fHasShownDisableAlert;
		bool				fRebootNeeded;
};

#endif /* DEVICESVIEW_H */
