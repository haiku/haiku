/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef EXTENSIONS_LIST_H
#define EXTENSIONS_LIST_H


#include <ColumnListView.h>
#include <String.h>


class ExtensionRow : public BRow {
public:
								ExtensionRow(const char* extensionName);
		virtual					~ExtensionRow();

private:
				BString			fExtensionName;
};


class ExtensionsList : public BColumnListView {
public:
								ExtensionsList();
		virtual					~ExtensionsList();
};


#endif	/* EXTENSIONS_LIST_H */
