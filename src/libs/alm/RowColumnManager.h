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


namespace BALM {

class RowColumnManager {
public:
								RowColumnManager(LinearSpec* spec);
								~RowColumnManager();

			void				AddArea(Area* area);
			void				RemoveArea(Area* area);

			void				UpdateConstraints();
			void				TabsChanged(Area* area);

			Row*				CreateRow(YTab* top, YTab* bottom);
			Column*				CreateColumn(XTab* left, XTab* right);
private:
			Row*				_FindRowFor(Area* area);
			Column*				_FindColumnFor(Area* area);

			double				_PreferredHeight(Row* row,
									double& weight);
			double				_PreferredWidth(Column* column,
									double& weight);

			void				_UpdateConstraints(Row* row);
			void				_UpdateConstraints(Column* column);

			BObjectList<Row>		fRows;
			BObjectList<Column>	fColumns;

			LinearSpec*			fLinearSpec;
};


} // namespace BALM


#endif // ROW_COLUMN_MANAGER_H

