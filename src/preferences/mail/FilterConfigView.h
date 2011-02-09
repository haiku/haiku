/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FILTER_CONFIG_VIEW_H
#define FILTER_CONFIG_VIEW_H


#include <vector>

#include <Box.h>
#include <Button.h>
#include <image.h>
#include <ListView.h>
#include <Message.h>
#include <MenuField.h>

#include "FilterAddonList.h"
#include "MailSettings.h"


class FilterConfigBox;


class FiltersConfigView : public BBox {
public:
								FiltersConfigView(BRect rect,
									BMailAccountSettings& account);
								~FiltersConfigView();

			void				AttachedToWindow();
			void				DetachedFromWindow();
			void				MessageReceived(BMessage *msg);

private:
			MailAddonSettings*	_GetCurrentMailSettings();
			FilterAddonList*	_GetCurrentFilterAddonList();

			void				_SelectFilter(int32 index);
			void				_SetDirection(direction direction);
			void				_SaveConfig(int32 index);

			BMailAccountSettings&	fAccount;
			direction			fDirection;

			FilterAddonList		fInboundFilters;
			FilterAddonList		fOutboundFilters;

			BMenuField*			fChainsField;
			BListView*			fListView;
			BMenuField*			fAddField;
			BButton*			fRemoveButton;
			FilterConfigBox*	fFilterView;

			int32				fCurrentIndex;
};

#endif //FILTER_CONFIG_VIEW_H
