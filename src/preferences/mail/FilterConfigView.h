/*
 * Copyright 2007-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FILTER_CONFIG_VIEW_H
#define FILTER_CONFIG_VIEW_H


#include <vector>

#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MailSettings.h>
#include <Message.h>
#include <MenuField.h>

#include "FilterList.h"


class FilterSettingsView;


class FiltersConfigView : public BBox {
public:
								FiltersConfigView(
									BMailAccountSettings& account);
								~FiltersConfigView();

			void				AttachedToWindow();
			void				DetachedFromWindow();
			void				MessageReceived(BMessage *msg);

private:
			BMailProtocolSettings* _MailSettings();
			::FilterList*		_FilterList();

			void				_SelectFilter(int32 index);
			void				_SetDirection(direction direction);
			void				_SaveConfig(int32 index);

private:
			BMailAccountSettings& fAccount;
			direction			fDirection;

			::FilterList		fInboundFilters;
			::FilterList		fOutboundFilters;

			BMenuField*			fChainsField;
			BListView*			fListView;
			BMenuField*			fAddField;
			BButton*			fRemoveButton;
			FilterSettingsView*	fFilterView;

			int32				fCurrentIndex;
};


#endif // FILTER_CONFIG_VIEW_H
