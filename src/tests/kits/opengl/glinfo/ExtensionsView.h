/*
 * Copyright 2009-2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXTENSIONS_VIEW_H
#define EXTENSIONS_VIEW_H


#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <GroupView.h>


class ExtensionsView : public BGroupView {
public:
								ExtensionsView();
		virtual					~ExtensionsView();

private:
				void			_AddExtensionsList(
									BColumnListView* fExtensionsList,
									char* stringList);

				BColumnListView* fExtensionsList;
				BStringColumn*	fAvailableColumn;
};


#endif	/* EXTENSIONS_VIEW_H */
