/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Pieter Panman
 */
#ifndef PROPERTYLIST_H
#define PROPERTYLIST_H


#include <ColumnListView.h>
#include <String.h>

#include "Device.h"

struct Attribute;

enum {
	kNameColumn,
	kValueColumn
};


class PropertyRow : public BRow {
public:
						PropertyRow(const char* name, const char* value);
	virtual				~PropertyRow();
		
			const char*	Name() const { return fName.String(); }
			const char*	Value() const { return fValue.String(); }
			void		SetName(const char* name);
			void		SetValue(const char* value);
private:
	BString		fName;
	BString		fValue;
};


class PropertyList : public BColumnListView {
public:
					PropertyList(const char* name);
	virtual			~PropertyList();
			void	RemoveAll();
			void	AddAttributes(const Attributes& attributes);
protected:
	virtual	void	SelectionChanged();
};

#endif /* PROPERTYLIST_H*/

