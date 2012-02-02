/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef	ROW_COLUMN_MANAGER_H
#define	ROW_COLUMN_MANAGER_H


#include "Area.h"
#include "Column.h"
#include "LinearSpec.h"
#include "Row.h"
#include "Tab.h"


namespace BPrivate {
	class SharedSolver;
};


namespace BALM {


/*! The RowColumnManager groups areas with same vertical or horizontal tabs
	into column and rows. For each row and column, a preferred size is
	calculated from the areas in the row or column. This preferred size is used
	to create a preferred size soft-constraint.
	Having only one constraint for each row and column avoids the so called
	spring effect. That is each area with a preferred size constraint is pulling
	or pressing torwards its preferred size. For example, a row with three areas
	pushes stronger than a row with two areas. Assuming that all areas have	the
	same preferred size, the three-area row gets a different size than the
	two-area row. However, one would expect that both rows have the same height.
	The row and column approach of the RowColumnManager solves this problem.

*/
class RowColumnManager {
public:
								RowColumnManager(LinearSpec* spec);
								~RowColumnManager();

			void				AddArea(Area* area);
			void				RemoveArea(Area* area);

			void				UpdateConstraints();
			void				TabsChanged(Area* area);
private:
			friend class BPrivate::SharedSolver;
			Row*				_FindRowFor(Area* area);
			Column*				_FindColumnFor(Area* area);

			double				_PreferredHeight(Row* row,
									double& weight);
			double				_PreferredWidth(Column* column,
									double& weight);

			void				_UpdateConstraints(Row* row);
			void				_UpdateConstraints(Column* column);

			BObjectList<Row>	fRows;
			BObjectList<Column>	fColumns;

			LinearSpec*			fLinearSpec;
};


} // namespace BALM


#endif // ROW_COLUMN_MANAGER_H

