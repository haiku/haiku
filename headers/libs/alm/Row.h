/*
 * Copyright 2007-2008, Christof Lutteroth, lutteroth@cs.auckland.ac.nz
 * Copyright 2007-2008, James Kim, jkim202@ec.auckland.ac.nz
 * Distributed under the terms of the MIT License.
 */

#ifndef	ROW_H
#define	ROW_H

#include "Constraint.h"

#include <List.h>


namespace BALM {

class BALMLayout;
class YTab;
	
/**
 * Represents a row defined by two y-tabs.
 */
class Row {
	
public:
	YTab*				Top() const;
	YTab*				Bottom() const;
	Row*				Previous() const;
	void					SetPrevious(Row* value);
	Row*				Next() const;
	void					SetNext(Row* value);
	//~ string				ToString();
	void					InsertBefore(Row* row);
	void					InsertAfter(Row* row);
	Constraint*			HasSameHeightAs(Row* row);
	BList*				Constraints() const;
	void					SetConstraints(BList* constraints);
						~Row();

protected:
						Row(BALMLayout* ls);

protected:
	BALMLayout*			fLS;
	YTab*				fTop;
	YTab*				fBottom;

private:
	Row*				fPrevious;
	Row*				fNext;
	Constraint*			fPreviousGlue;
	Constraint*			fNextGlue;
	BList*				fConstraints;

public:
	friend class			BALMLayout;
	
};

}	// namespace BALM

using BALM::Row;

#endif	// ROW_H
