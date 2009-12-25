/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "main_window/ThreadsPage.h"

#include <stdio.h>

#include <new>

#include "table/TableColumns.h"


// #pragma mark - ThreadsTableModel


class MainWindow::ThreadsPage::ThreadsTableModel : public TableModel {
public:
	ThreadsTableModel(Model* model)
		:
		fModel(model)
	{
	}

	virtual int32 CountColumns() const
	{
		return 16;
	}

	virtual int32 CountRows() const
	{
		return fModel->CountThreads();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		Model::Thread* thread = fModel->ThreadAt(rowIndex);
		if (thread == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(thread->ID());
				return true;
			case 1:
				value.SetTo(thread->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 2:
			{
				char buffer[128];
				Model::Team* team = thread->GetTeam();
				snprintf(buffer, sizeof(buffer), "%s (%ld)", team->Name(),
					team->ID());
				value.SetTo(buffer);
				return true;
			}
			case 3:
				value.SetTo(thread->CreationTime());
				return true;
			case 4:
				value.SetTo(thread->DeletionTime());
				return true;
			case 5:
				value.SetTo(thread->Runs());
				return true;
			case 6:
				value.SetTo(thread->TotalRunTime());
				return true;
			case 7:
				value.SetTo(thread->Latencies());
				return true;
			case 8:
				value.SetTo(thread->TotalLatency());
				return true;
			case 9:
				value.SetTo(thread->Preemptions());
				return true;
			case 10:
				value.SetTo(thread->TotalRerunTime());
				return true;
			case 11:
				value.SetTo(thread->Waits());
				return true;
			case 12:
				value.SetTo(thread->TotalWaitTime());
				return true;
			case 13:
				value.SetTo(thread->UnspecifiedWaitTime());
				return true;
			case 14:
				value.SetTo(thread->IOCount());
				return true;
			case 15:
				value.SetTo(thread->IOTime());
				return true;
			default:
				return false;
		}
	}

private:
	Model*	fModel;
};


// #pragma mark - ThreadsPage


MainWindow::ThreadsPage::ThreadsPage(MainWindow* parent)
	:
	BGroupView(B_VERTICAL),
	fParent(parent),
	fThreadsTable(NULL),
	fThreadsTableModel(NULL),
	fModel(NULL)
{
	SetName("Threads");

	fThreadsTable = new Table("threads list", 0);
	AddChild(fThreadsTable->ToView());

	fThreadsTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fThreadsTable->AddColumn(new StringTableColumn(2, "Team", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(3, "Creation", 80, 40,
		1000, true, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(4, "Deletion", 80, 40,
		1000, false, B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(5, "Runs", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(6, "Run time", 80, 20,
		1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(7, "Latencies", 80, 20, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(8, "Latency time", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(9, "Preemptions", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(10, "Preemption time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(11, "Waits", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(12, "Wait time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(13, "Unspecified time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new Int64TableColumn(14, "I/O Count", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new NanotimeTableColumn(15, "I/O Time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));

	fThreadsTable->AddTableListener(this);
}


MainWindow::ThreadsPage::~ThreadsPage()
{
	fThreadsTable->SetTableModel(NULL);
	delete fThreadsTableModel;
}


void
MainWindow::ThreadsPage::SetModel(Model* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fThreadsTable->SetTableModel(NULL);
		delete fThreadsTableModel;
		fThreadsTableModel = NULL;
	}

	fModel = model;

	if (fModel != NULL) {
		fThreadsTableModel = new(std::nothrow) ThreadsTableModel(fModel);
		fThreadsTable->SetTableModel(fThreadsTableModel);
		fThreadsTable->ResizeAllColumnsToPreferred();
	}
}


void
MainWindow::ThreadsPage::TableRowInvoked(Table* table, int32 rowIndex)
{
	Model::Thread* thread = fModel->ThreadAt(rowIndex);
	if (thread != NULL)
		fParent->OpenThreadWindow(thread);
}
