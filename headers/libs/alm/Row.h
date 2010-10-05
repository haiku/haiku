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

class BALMLayout;

/**
 * Represents a row defined by two y-tabs.
 */
class Row {
public:
								~Row();

			YTab*				Top() const;
			YTab*				Bottom() const;
			Row*				Previous() const;
			void				SetPrevious(Row* value);
			Row*				Next() const;
			void				SetNext(Row* value);
			void				InsertBefore(Row* row);
			void				InsertAfter(Row* row);
			Constraint*			HasSameHeightAs(Row* row);
			ConstraintList*		Constraints() const;

protected:
								Row(BALMLayout* layout);

protected:
			LinearSpec*			fLS;
			YTab*				fTop;
			YTab*				fBottom;

private:
			Row*				fPrevious;
			Row*				fNext;
			Constraint*			fPreviousGlue;
			Constraint*			fNextGlue;
			ConstraintList		fConstraints;

public:
	friend class			BALMLayout;
	
};

}	// namespace BALM

using BALM::Row;

#endif	// ROW_H
