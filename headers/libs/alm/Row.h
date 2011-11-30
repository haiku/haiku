/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ROW_H
#define	ROW_H


#include "Constraint.h"
#include "LinearSpec.h"
#include "Tab.h"


namespace BALM {


class Area;
class BALMLayout;
class RowColumnManager;


/**
 * Represents a row defined by two y-tabs.
 */
class Row {
public:
								~Row();

			YTab*				Top() const;
			YTab*				Bottom() const;

private:
								Row(LinearSpec* ls, YTab* top, YTab* bottom);

			LinearSpec*			fLS;
			BReference<YTab>	fTop;
			BReference<YTab>	fBottom;

			//! managed by RowColumnManager
			Constraint*			fPrefSizeConstraint;
			BObjectList<Area>	fAreas;

public:
	friend class BALMLayout;
	friend class BALM::RowColumnManager;
	
};

}	// namespace BALM

using BALM::Row;

#endif	// ROW_H
