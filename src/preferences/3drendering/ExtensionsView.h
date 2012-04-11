/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXTENSIONS_VIEW_H
#define EXTENSIONS_VIEW_H


#include <GroupView.h>


class ExtensionsList;

class ExtensionsView : public BGroupView {
public:
								ExtensionsView();
		virtual					~ExtensionsView();

private:
				void			_AddExtensionsList(ExtensionsList *extList,
									char* stringList);
};


#endif	/* EXTENSIONS_VIEW_H */
