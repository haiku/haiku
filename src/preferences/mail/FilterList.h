/*
 * Copyright 2011-2012, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FILTER_LIST_H
#define FILTER_LIST_H


#include <MailSettings.h>
#include <View.h>


enum direction {
	kIncoming,
	kOutgoing
};


struct FilterInfo {
	image_id		image;
	entry_ref		ref;
};


class FilterList {
public:
								FilterList(direction dir,
									bool loadOnStart = true);
								~FilterList();

			void				Reload();

			int32				CountInfos();
			FilterInfo&			InfoAt(int32 index);

			bool				GetDescriptiveName(int32 index, BString& name);
			bool				GetDescriptiveName(const entry_ref& ref,
									BString& name);

			BView*				CreateConfigView(BMailAddOnSettings& settings);

			int32				InfoIndexFor(const entry_ref& ref);

private:
			void				_MakeEmpty();
			void				_LoadAddOn(BEntry& entry);

			direction			fDirection;
			std::vector<FilterInfo> fList;
};


#endif // FILTER_LIST_H
