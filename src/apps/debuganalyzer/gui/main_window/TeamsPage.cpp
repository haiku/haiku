/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "main_window/TeamsPage.h"

#include <stdio.h>

#include <new>

#include "table/TableColumns.h"


// #pragma mark - TeamsTableModel


class MainWindow::TeamsPage::TeamsTableModel : public TableModel {
public:
	TeamsTableModel(Model* model)
		:
		fModel(model)
	{
	}

	virtual int32 CountColumns() const
	{
		return 4;
	}

	virtual int32 CountRows() const
	{
		return fModel->CountTeams();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		Model::Team* team = fModel->TeamAt(rowIndex);
		if (team == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(team->ID());
				return true;
			case 1:
				value.SetTo(team->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 2:
				value.SetTo(team->CreationTime());
				return true;
			case 3:
				value.SetTo(team->DeletionTime());
				return true;
			default:
				return false;
		}
	}

private:
	Model*	fModel;
};


// #pragma mark - TeamsPage


MainWindow::TeamsPage::TeamsPage(MainWindow* parent)
	:
	BGroupView(B_VERTICAL),
	fParent(parent),
	fTeamsTable(NULL),
	fTeamsTableModel(NULL),
	fModel(NULL)
{
	SetName("Teams");

	fTeamsTable = new Table("teams list", 0);
	AddChild(fTeamsTable->ToView());

	fTeamsTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fTeamsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fTeamsTable->AddColumn(new NanotimeTableColumn(2, "Creation", 80, 40, 1000,
		true, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fTeamsTable->AddColumn(new NanotimeTableColumn(3, "Deletion", 80, 40, 1000,
		false, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));

	fTeamsTable->AddTableListener(this);
}


MainWindow::TeamsPage::~TeamsPage()
{
	fTeamsTable->SetTableModel(NULL);
	delete fTeamsTableModel;
}


void
MainWindow::TeamsPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fTeamsTable->SetTableModel(NULL);
		delete fTeamsTableModel;
		fTeamsTableModel = NULL;
	}

	fModel = model;

	if (fModel != NULL) {
		fTeamsTableModel = new(std::nothrow) TeamsTableModel(fModel);
		fTeamsTable->SetTableModel(fTeamsTableModel);
		fTeamsTable->ResizeAllColumnsToPreferred();
	}
}


void
MainWindow::TeamsPage::TableRowInvoked(Table* table, int32 rowIndex)
{
	Model::Team* team = fModel->TeamAt(rowIndex);
	if (team != NULL)
		fParent->OpenTeamWindow(team);
}
