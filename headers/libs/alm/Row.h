/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ROW_H
#define	ROW_H


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
 * Represents a row defined by two y-tabs.
 */
class Row {
public:
								~Row();

			YTab*				Top() const;
			YTab*				Bottom() const;

private:
								Row(LinearProgramming::LinearSpec* ls,
										YTab* top, YTab* bottom);

			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;

			LinearProgramming::LinearSpec* fLS;
			LinearProgramming::Constraint* fPrefSizeConstraint;
				// managed by RowColumnManager

			BObjectList<Area>	fAreas;

public:
	friend class BALMLayout;
	friend class BALM::RowColumnManager;
	
};

}	// namespace BALM

using BALM::Row;

#endif	// ROW_H
