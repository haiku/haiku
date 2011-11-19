/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BreakpointListView.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "FunctionID.h"
#include "GUISettingsUtils.h"
#include "LocatableFile.h"
#include "table/TableColumns.h"
#include "Team.h"
#include "UserBreakpoint.h"
#include "TargetAddressTableColumn.h"


// #pragma mark - BreakpointsTableModel


class BreakpointListView::BreakpointsTableModel : public TableModel {
public:
	BreakpointsTableModel(Team* team)
		:
		fTeam(team)
	{
		Update(NULL);
	}

	~BreakpointsTableModel()
	{
		fTeam = NULL;
		Update(NULL);
	}

	bool Update(UserBreakpoint* changedBreakpoint)
	{
		if (fTeam == NULL) {
			for (int32 i = 0;
				UserBreakpoint* breakpoint = fBreakpoints.ItemAt(i);
				i++) {
				breakpoint->ReleaseReference();
			}
			fBreakpoints.MakeEmpty();

			return true;
		}

		AutoLocker<Team> locker(fTeam);

		UserBreakpointList::ConstIterator it
			= fTeam->UserBreakpoints().GetIterator();
		UserBreakpoint* newBreakpoint = it.Next();
		int32 index = 0;

		// remove no longer existing breakpoints
		while (UserBreakpoint* oldBreakpoint = fBreakpoints.ItemAt(index)) {
			if (oldBreakpoint == newBreakpoint) {
				if (oldBreakpoint == changedBreakpoint)
					NotifyRowsChanged(index, 1);
				index++;
				newBreakpoint = it.Next();
			} else {
				// TODO: Not particularly efficient!
				fBreakpoints.RemoveItemAt(index);
				oldBreakpoint->ReleaseReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new breakpoints
		int32 countBefore = fBreakpoints.CountItems();
		while (newBreakpoint != NULL) {
			if (!fBreakpoints.AddItem(newBreakpoint))
				return false;

			newBreakpoint->AcquireReference();
			newBreakpoint = it.Next();
		}

		int32 count = fBreakpoints.CountItems();
		if (count > countBefore)
			NotifyRowsAdded(countBefore, count - countBefore);

		return true;
	}

	virtual int32 CountColumns() const
	{
		return 5;
	}

	virtual int32 CountRows() const
	{
		return fBreakpoints.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		UserBreakpoint* breakpoint = fBreakpoints.ItemAt(rowIndex);
		if (breakpoint == NULL)
			return false;
		const UserBreakpointLocation& location = breakpoint->Location();

		switch (columnIndex) {
			case 0:
				value.SetTo((int32)breakpoint->IsEnabled());
				return true;
			case 1:
				value.SetTo(location.GetFunctionID()->FunctionName(),
					B_VARIANT_DONT_COPY_DATA);
				return true;
			case 2:
				if (LocatableFile* sourceFile = location.SourceFile()) {
					value.SetTo(sourceFile->Name(), B_VARIANT_DONT_COPY_DATA);
					return true;
				}
				return false;
			case 3:
				if (location.SourceFile() != NULL) {
					value.SetTo(location.GetSourceLocation().Line() + 1);
					return true;
				}
				return false;
			case 4:
				if (location.SourceFile() == NULL) {
					AutoLocker<Team> teamLocker(fTeam);
					if (UserBreakpointInstance* instance
							= breakpoint->InstanceAt(0)) {
						value.SetTo(instance->Address());
						return true;
					}
				}
				return false;
			default:
				return false;
		}
	}

	UserBreakpoint* BreakpointAt(int32 index) const
	{
		return fBreakpoints.ItemAt(index);
	}

private:
	Team*						fTeam;
	BObjectList<UserBreakpoint>	fBreakpoints;
};


// #pragma mark - BreakpointListView


BreakpointListView::BreakpointListView(Team* team, Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fBreakpoint(NULL),
	fBreakpointsTable(NULL),
	fBreakpointsTableModel(NULL),
	fListener(listener)
{
}


BreakpointListView::~BreakpointListView()
{
	fBreakpointsTable->SetTableModel(NULL);
	delete fBreakpointsTableModel;
}


/*static*/ BreakpointListView*
BreakpointListView::Create(Team* team, Listener* listener)
{
	BreakpointListView* self = new BreakpointListView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
BreakpointListView::UnsetListener()
{
	fListener = NULL;
}


void
BreakpointListView::SetBreakpoint(UserBreakpoint* breakpoint)
{
	if (breakpoint == fBreakpoint)
		return;

	if (fBreakpoint != NULL)
		fBreakpoint->ReleaseReference();

	fBreakpoint = breakpoint;

	if (fBreakpoint != NULL) {
		fBreakpoint->AcquireReference();

		for (int32 i = 0;
			UserBreakpoint* other = fBreakpointsTableModel->BreakpointAt(i);
			i++) {
			if (fBreakpoint == other) {
				fBreakpointsTable->SelectRow(i, false);
				return;
			}
		}
	}

	fBreakpointsTable->DeselectAllRows();
}


void
BreakpointListView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	fBreakpointsTableModel->Update(breakpoint);
}


void
BreakpointListView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("breakpointsTable", &tableSettings) == B_OK) {
		GUISettingsUtils::UnarchiveTableSettings(tableSettings,
			fBreakpointsTable);
	}
}


status_t
BreakpointListView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GUISettingsUtils::ArchiveTableSettings(tableSettings,
		fBreakpointsTable);
	if (result == B_OK)
		result = settings.AddMessage("breakpointsTable", &tableSettings);

	return result;
}


void
BreakpointListView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL)
		return;

	TableSelectionModel* selectionModel = table->SelectionModel();
	UserBreakpoint* breakpoint = fBreakpointsTableModel->BreakpointAt(
		selectionModel->RowAt(0));

	fListener->BreakpointSelectionChanged(breakpoint);
}


void
BreakpointListView::_Init()
{
	fBreakpointsTable = new Table("breakpoints list", 0, B_FANCY_BORDER);
	AddChild(fBreakpointsTable->ToView());

	// columns
	fBreakpointsTable->AddColumn(new BoolStringTableColumn(0, "State", 70, 20,
		1000, "Enabled", "Disabled"));
	fBreakpointsTable->AddColumn(new StringTableColumn(1, "Function", 250, 40,
		1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fBreakpointsTable->AddColumn(new StringTableColumn(2, "File", 250, 40,
		1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fBreakpointsTable->AddColumn(new Int32TableColumn(3, "Line", 60, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fBreakpointsTable->AddColumn(new TargetAddressTableColumn(4, "Address", 100,
		20, 1000, B_TRUNCATE_END, B_ALIGN_RIGHT));

	fBreakpointsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fBreakpointsTable->AddTableListener(this);

	fBreakpointsTableModel = new BreakpointsTableModel(fTeam);
	fBreakpointsTable->SetTableModel(fBreakpointsTableModel);
}


// #pragma mark - Listener


BreakpointListView::Listener::~Listener()
{
}
