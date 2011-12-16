/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	COLUMN_H
#define	COLUMN_H


#include "Constraint.h"
#include "LinearSpec.h"
#include "Tab.h"


namespace BALM {


class Area;
class BALMLayout;
class RowColumnManager;


/**
 * Represents a column defined by two x-tabs.
 */
class Column {
public:
								~Column();

			XTab*				Left() const;
			XTab*				Right() const;

private:
								Column(LinearSpec* ls, XTab* left, XTab* right);

			LinearSpec*			fLS;
			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;

			//! managed by RowColumnManager
			Constraint*			fPrefSizeConstraint;
			BObjectList<Area>	fAreas;

public:
	friend class BALMLayout;
	friend class BALM::RowColumnManager;
	
};

}	// namespace BALM

using BALM::Column;

#endif	// COLUMN_H
