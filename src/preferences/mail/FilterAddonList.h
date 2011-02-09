/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FILTER_ADDON_LIST_H
#define FILTER_ADDON_LIST_H


#include "MailSettings.h"

#include <View.h>


enum direction {
	kIncomming,
	kOutgoing
};


struct FilterAddonInfo {
	image_id		image;
	entry_ref		ref;
};


class FilterAddonList {
public:
								FilterAddonList(direction dir,
									bool loadOnStart = true);
								~FilterAddonList();

			void				Reload();

			int32				CountFilterAddons();
			FilterAddonInfo&	FilterAddonAt(int32 index);

			bool				GetDescriptiveName(int32 index, BString& name);
			bool				GetDescriptiveName(const entry_ref& ref,
									BString& name);

			BView*				CreateConfigView(AddonSettings& settings);

			int32				FindInfo(const entry_ref& ref);
private:
			void				_MakeEmpty();
			void				_LoadAddon(BEntry& entry);

			direction				fDirection;
			std::vector<FilterAddonInfo>	fFilterAddonList;
};


#endif //FILTER_ADDON_LIST_H
