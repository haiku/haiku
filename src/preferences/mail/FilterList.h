/*
 * Copyright 2011-2015, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FILTER_LIST_H
#define FILTER_LIST_H


#include <MailSettings.h>
#include <MailSettingsView.h>


enum direction {
	kIncoming,
	kOutgoing
};


struct FilterInfo {
	image_id		image;
	entry_ref		ref;
	BMailSettingsView* (*instantiateSettingsView)(
		const BMailAccountSettings& accountSettings,
		const BMailAddOnSettings& settings);
	BString			(*name)(
		const BMailAccountSettings& accountSettings,
		const BMailAddOnSettings* settings);
};


class FilterList {
public:
								FilterList(direction dir);
								~FilterList();

			void				Reload();

			int32				CountInfos() const;
			const FilterInfo&	InfoAt(int32 index) const;
			int32				InfoIndexFor(const entry_ref& ref) const;

			BString				SimpleName(int32 index,
									const BMailAccountSettings& settings) const;
			BString				SimpleName(const entry_ref& ref,
									const BMailAccountSettings& settings) const;
			BString				DescriptiveName(int32 index,
									const BMailAccountSettings& accountSettings,
									const BMailAddOnSettings* settings) const;
			BString				DescriptiveName(const entry_ref& ref,
									const BMailAccountSettings& accountSettings,
									const BMailAddOnSettings* settings) const;

			BMailSettingsView*	CreateSettingsView(
									const BMailAccountSettings& accountSettings,
									const BMailAddOnSettings& settings);

private:
			void				_MakeEmpty();
			status_t			_LoadAddOn(BEntry& entry);

private:
			direction			fDirection;
			std::vector<FilterInfo> fList;
};


#endif // FILTER_LIST_H
