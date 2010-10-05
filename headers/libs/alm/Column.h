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

class BALMLayout;

/**
 * Represents a column defined by two x-tabs.
 */
class Column {
public:
								~Column();

			XTab*				Left() const;
			XTab*				Right() const;
			Column*				Previous() const;
			void				SetPrevious(Column* value);
			Column*				Next() const;
			void				SetNext(Column* value);

			void				InsertBefore(Column* column);
			void				InsertAfter(Column* column);
			Constraint*			HasSameWidthAs(Column* column);

			ConstraintList*		Constraints() const;

protected:
								Column(BALMLayout* layout);

protected:
			LinearSpec*			fLS;
			XTab*				fLeft;
			XTab*				fRight;

private:
			Column*				fPrevious;
			Column*				fNext;
			Constraint*			fPreviousGlue;
			Constraint*			fNextGlue;
			ConstraintList		fConstraints;

public:
	friend class			BALMLayout;
	
};

}	// namespace BALM

using BALM::Column;

#endif	// COLUMN_H
