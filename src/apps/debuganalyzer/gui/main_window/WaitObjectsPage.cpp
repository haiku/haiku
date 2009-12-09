/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "main_window/WaitObjectsPage.h"

#include <stdio.h>

#include <new>

#include <thread_defs.h>

#include "table/TableColumns.h"


// #pragma mark - WaitObjectsTableModel


class MainWindow::WaitObjectsPage::WaitObjectsTableModel : public TableModel {
public:
	WaitObjectsTableModel(Model* model)
		:
		fModel(model)
	{
	}

	virtual int32 CountColumns() const
	{
		return 6;
	}

	virtual int32 CountRows() const
	{
		return fModel->CountWaitObjectGroups();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		Model::WaitObjectGroup* group = fModel->WaitObjectGroupAt(rowIndex);
		if (group == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(wait_object_type_name(group->Type()),
					B_VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				value.SetTo(group->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 4:
				value.SetTo(group->Waits());
				return true;
			case 5:
				value.SetTo(group->TotalWaitTime());
				return true;
			default:
				return false;
		}
	}

private:
	Model*	fModel;
};


// #pragma mark - WaitObjectsPage


MainWindow::WaitObjectsPage::WaitObjectsPage(MainWindow* parent)
	:
	BGroupView(B_VERTICAL),
	fParent(parent),
	fWaitObjectsTable(NULL),
	fWaitObjectsTableModel(NULL),
	fModel(NULL)
{
	SetName("WaitObjects");

	fWaitObjectsTable = new Table("waiting objects list", 0);
	AddChild(fWaitObjectsTable->ToView());

	fWaitObjectsTable->AddColumn(new StringTableColumn(0, "Type", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
//	fWaitObjectsTable->AddColumn(new StringTableColumn(2, "Object", 80, 40, 1000,
//		B_TRUNCATE_END, B_ALIGN_LEFT));
//	fWaitObjectsTable->AddColumn(new StringTableColumn(3, "Referenced", 80, 40,
//		1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTable->AddColumn(new Int64TableColumn(4, "Waits", 80, 40,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fWaitObjectsTable->AddColumn(new NanotimeTableColumn(5, "Wait Time", 80,
		40, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
}


MainWindow::WaitObjectsPage::~WaitObjectsPage()
{
	fWaitObjectsTable->SetTableModel(NULL);
	delete fWaitObjectsTableModel;
}


void
MainWindow::WaitObjectsPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fWaitObjectsTable->SetTableModel(NULL);
		delete fWaitObjectsTableModel;
		fWaitObjectsTableModel = NULL;
	}

	fModel = model;

	if (fModel != NULL) {
		fWaitObjectsTableModel
			= new(std::nothrow) WaitObjectsTableModel(fModel);
		fWaitObjectsTable->SetTableModel(fWaitObjectsTableModel);
		fWaitObjectsTable->ResizeAllColumnsToPreferred();
	}
}
