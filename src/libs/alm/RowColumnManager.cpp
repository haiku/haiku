/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "RowColumnManager.h"


#include <LayoutItem.h>


using namespace LinearProgramming;


namespace BALM {


RowColumnManager::RowColumnManager(LinearSpec* spec)
	:
	fLinearSpec(spec)
{

}


RowColumnManager::~RowColumnManager()
{
	for (int32 i = 0; i < fRows.CountItems(); i++)
		delete fRows.ItemAt(i)->fPrefSizeConstraint;

	for (int32 i = 0; i < fColumns.CountItems(); i++)
		delete fColumns.ItemAt(i)->fPrefSizeConstraint;
}

					
void
RowColumnManager::AddArea(Area* area)
{
	Row* row = _FindRowFor(area);
	if (row == NULL) {
		row = new Row(fLinearSpec, area->Top(), area->Bottom());
		fRows.AddItem(row);
	}
	area->fRow = row;
	row->fAreas.AddItem(area);

	Column* column = _FindColumnFor(area);
	if (column == NULL) {
		column = new Column(fLinearSpec, area->Left(), area->Right());
		fColumns.AddItem(column);
	}
	area->fColumn = column;
	column->fAreas.AddItem(area);

	_UpdateConstraints(row);
	_UpdateConstraints(column);
}


void
RowColumnManager::RemoveArea(Area* area)
{
	Row* row = area->fRow;
	if (row) {
		row->fAreas.RemoveItem(area);
		area->fRow = NULL;
		if (row->fAreas.CountItems() == 0) {
			fRows.RemoveItem(row);
			delete row;
		} else
			_UpdateConstraints(row);
	}

	Column* column = area->fColumn;
	if (column) {
		column->fAreas.RemoveItem(area);
		area->fColumn = NULL;
		if (column->fAreas.CountItems() == 0) {
			fColumns.RemoveItem(column);
			delete column;
		} else
			_UpdateConstraints(column);
	}
}


void
RowColumnManager::UpdateConstraints()
{
	for (int32 i = 0; i < fRows.CountItems(); i++)
		_UpdateConstraints(fRows.ItemAt(i));
	for (int32 i = 0; i < fColumns.CountItems(); i++)
		_UpdateConstraints(fColumns.ItemAt(i));
}


void
RowColumnManager::TabsChanged(Area* area)
{
	RemoveArea(area);
	AddArea(area);
}


Row*
RowColumnManager::_FindRowFor(Area* area)
{
	for (int32 i = 0; i < fRows.CountItems(); i++) {
		Row* row = fRows.ItemAt(i);
		if (row->fTop.Get() == area->Top()
			&& row->fBottom.Get() == area->Bottom())
			return row;
	}
	return NULL;
}


Column*
RowColumnManager::_FindColumnFor(Area* area)
{
	for (int32 i = 0; i < fColumns.CountItems(); i++) {
		Column* column = fColumns.ItemAt(i);
		if (column->fLeft.Get() == area->Left()
			&& column->fRight.Get() == area->Right())
			return column;
	}
	return NULL;
}


double
RowColumnManager::_PreferredHeight(Row* row, double& weight)
{
	weight = 0;
	int nAreas = 0;
	double pref = 0;
	for (int32 i = 0; i < row->fAreas.CountItems(); i++) {
		BSize prefSize = row->fAreas.ItemAt(i)->Item()->PreferredSize();
		if (prefSize.height <= 0)
			continue;
		nAreas++;
		pref += prefSize.height;
		double negPen = row->fAreas.ItemAt(i)->ShrinkPenalties().height;
		if (negPen > 0)
			weight += negPen;
	}
	if (nAreas == 0) {
		pref = -1;
		weight = 1;
	} else {
		pref /= nAreas;
		weight /= nAreas;
	}
	return pref;
}


double
RowColumnManager::_PreferredWidth(Column* column, double& weight)
{
	weight = 0;
	int nAreas = 0;
	double pref = 0;
	for (int32 i = 0; i < column->fAreas.CountItems(); i++) {
		BSize prefSize = column->fAreas.ItemAt(i)->Item()->PreferredSize();
		if (prefSize.width <= 0)
			continue;
		nAreas++;
		pref += prefSize.width;

		double negPen = column->fAreas.ItemAt(i)->ShrinkPenalties().height;
		if (negPen > 0)
			weight += negPen;
	}
	if (nAreas == 0) {
		pref = -1;
		weight = 1;
	} else {
		pref /= nAreas;
		weight /= nAreas;
	}
	return pref;
}


void
RowColumnManager::_UpdateConstraints(Row* row)
{
	double weight;
	double prefSize = _PreferredHeight(row, weight);
	if (prefSize >= 0) {
		if (row->fPrefSizeConstraint == NULL) {
			row->fPrefSizeConstraint = fLinearSpec->AddConstraint(1,
				row->fBottom, -1, row->fTop, kEQ, prefSize, weight, weight);
			row->fPrefSizeConstraint->SetLabel("Pref Height");
		} else {
			row->fPrefSizeConstraint->SetRightSide(prefSize);
			row->fPrefSizeConstraint->SetPenaltyNeg(weight);
			row->fPrefSizeConstraint->SetPenaltyPos(weight);
		}
	} else {
		delete row->fPrefSizeConstraint;
		row->fPrefSizeConstraint = NULL;
	}
}


void
RowColumnManager::_UpdateConstraints(Column* column)
{
	double weight;
	double prefSize = _PreferredWidth(column, weight);
	if (prefSize >= 0) {
		if (column->fPrefSizeConstraint == NULL) {
			column->fPrefSizeConstraint = fLinearSpec->AddConstraint(1,
				column->fRight, -1, column->fLeft, kEQ, prefSize, weight,
				weight);
			column->fPrefSizeConstraint->SetLabel("Pref Width");
		} else {
			column->fPrefSizeConstraint->SetRightSide(prefSize);
			column->fPrefSizeConstraint->SetPenaltyNeg(weight);
			column->fPrefSizeConstraint->SetPenaltyPos(weight);
		}
	} else {
		delete column->fPrefSizeConstraint;
		column->fPrefSizeConstraint = NULL;
	}
}


} // end BALM namespace

