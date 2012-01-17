/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	COLUMN_H
#define	COLUMN_H


#include <ObjectList.h>
#include <Referenceable.h>


namespace LinearProgramming {
	class Constraint;
	class LinearSpec;
};


namespace BALM {


class Area;
class BALMLayout;
class RowColumnManager;
class XTab;
class YTab;


/**
 * Represents a column defined by two x-tabs.
 */
class Column {
public:
								~Column();

			XTab*				Left() const;
			XTab*				Right() const;

private:
								Column(LinearProgramming::LinearSpec* ls,
									XTab* left, XTab* right);

			BReference<XTab>	fLeft;
			BReference<XTab>	fRight;

			LinearProgramming::LinearSpec* fLS;
			LinearProgramming::Constraint* fPrefSizeConstraint;
				// managed by RowColumnManager

			BObjectList<Area>	fAreas;

public:
	friend class BALMLayout;
	friend class BALM::RowColumnManager;
	
};

}	// namespace BALM

using BALM::Column;

#endif	// COLUMN_H
