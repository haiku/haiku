/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "BreakpointListView.h"

#include <stdio.h>

#include <new>

#include <MessageFilter.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "Architecture.h"
#include "FunctionID.h"
#include "GuiSettingsUtils.h"
#include "LocatableFile.h"
#include "MessageCodes.h"
#include "table/TableColumns.h"
#include "TargetAddressTableColumn.h"
#include "Team.h"
#include "UserBreakpoint.h"
#include "Watchpoint.h"


// #pragma mark - BreakpointProxy


BreakpointProxy::BreakpointProxy(UserBreakpoint* breakpoint,
	Watchpoint* watchpoint)
	:
	fBreakpoint(breakpoint),
	fWatchpoint(watchpoint)
{
	if (fBreakpoint != NULL)
		fBreakpoint->AcquireReference();

	if (fWatchpoint != NULL)
		fWatchpoint->AcquireReference();
}


BreakpointProxy::~BreakpointProxy()
{
	if (fBreakpoint != NULL)
		fBreakpoint->ReleaseReference();

	if (fWatchpoint != NULL)
		fWatchpoint->ReleaseReference();
}


breakpoint_proxy_type
BreakpointProxy::Type() const
{
	return fBreakpoint != NULL ? BREAKPOINT_PROXY_TYPE_BREAKPOINT
			: BREAKPOINT_PROXY_TYPE_WATCHPOINT;
}


// #pragma mark - ListInputFilter


class BreakpointListView::ListInputFilter : public BMessageFilter {
public:
	ListInputFilter(BView* view)
		:
		BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN),
		fTargetView(view)
	{
	}

	~ListInputFilter()
	{
	}

	filter_result Filter(BMessage* message, BHandler** target)
	{
		const char* bytes;
		if (message->FindString("bytes", &bytes) == B_OK
			&& bytes[0] == B_DELETE) {
			BMessenger(fTargetView).SendMessage(MSG_CLEAR_BREAKPOINT);
		}

		return B_DISPATCH_MESSAGE;
	}

private:
	BView*			fTargetView;
};


// #pragma mark - BreakpointsTableModel


class BreakpointListView::BreakpointsTableModel : public TableModel {
public:
	BreakpointsTableModel(Team* team)
		:
		fTeam(team)
	{
		UpdateBreakpoint(NULL);
	}

	~BreakpointsTableModel()
	{
		fTeam = NULL;
		UpdateBreakpoint(NULL);
	}

	bool UpdateBreakpoint(BreakpointProxy* proxy)
	{
		if (fTeam == NULL) {
			for (int32 i = 0;
				BreakpointProxy* proxy = fBreakpointProxies.ItemAt(i);
				i++) {
				proxy->ReleaseReference();
			}
			fBreakpointProxies.MakeEmpty();

			return true;
		}

		AutoLocker<Team> locker(fTeam);

		UserBreakpointList::ConstIterator it
			= fTeam->UserBreakpoints().GetIterator();
		int32 watchpointIndex = 0;
		UserBreakpoint* newBreakpoint = it.Next();
		Watchpoint* newWatchpoint = fTeam->WatchpointAt(watchpointIndex);
		int32 index = 0;
		bool remove;

		// remove no longer existing breakpoints
		while (BreakpointProxy* oldProxy = fBreakpointProxies.ItemAt(index)) {
			remove = false;
			switch (oldProxy->Type()) {
				case BREAKPOINT_PROXY_TYPE_BREAKPOINT:
				{
					UserBreakpoint* breakpoint = oldProxy->GetBreakpoint();
					if (breakpoint == newBreakpoint) {
						if (breakpoint == proxy->GetBreakpoint())
							NotifyRowsChanged(index, 1);
						++index;
						newBreakpoint = it.Next();
					} else
						remove = true;
				}
				break;

				case BREAKPOINT_PROXY_TYPE_WATCHPOINT:
				{
					Watchpoint* watchpoint = oldProxy->GetWatchpoint();
					if (watchpoint == newWatchpoint) {
						if (watchpoint == proxy->GetWatchpoint())
							NotifyRowsChanged(index, 1);
						++watchpointIndex;
						++index;
						newWatchpoint = fTeam->WatchpointAt(watchpointIndex);
					} else
						remove = true;
				}
				break;
			}

			if (remove) {
				// TODO: Not particularly efficient!
				fBreakpointProxies.RemoveItemAt(index);
				oldProxy->ReleaseReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new breakpoints
		int32 countBefore = fBreakpointProxies.CountItems();
		BreakpointProxy* newProxy = NULL;
		BReference<BreakpointProxy> proxyReference;
		while (newBreakpoint != NULL) {
			newProxy = new(std::nothrow) BreakpointProxy(newBreakpoint, NULL);
			if (newProxy == NULL)
				return false;

			proxyReference.SetTo(newProxy, true);
			if (!fBreakpointProxies.AddItem(newProxy))
				return false;

			proxyReference.Detach();
			newBreakpoint = it.Next();
		}

		// add new watchpoints
		while (newWatchpoint != NULL) {
			newProxy = new(std::nothrow) BreakpointProxy(NULL, newWatchpoint);
			if (newProxy == NULL)
				return false;

			proxyReference.SetTo(newProxy, true);
			if (!fBreakpointProxies.AddItem(newProxy))
				return false;

			proxyReference.Detach();
			newWatchpoint = fTeam->WatchpointAt(++watchpointIndex);
		}


		int32 count = fBreakpointProxies.CountItems();
		if (count > countBefore)
			NotifyRowsAdded(countBefore, count - countBefore);

		return true;
	}

	virtual int32 CountColumns() const
	{
		return 3;
	}

	virtual int32 CountRows() const
	{
		return fBreakpointProxies.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		BreakpointProxy* proxy = fBreakpointProxies.ItemAt(rowIndex);
		if (proxy == NULL)
			return false;

		if (proxy->Type() == BREAKPOINT_PROXY_TYPE_BREAKPOINT) {
			return _GetBreakpointValueAt(proxy->GetBreakpoint(), rowIndex,
				columnIndex, value);
		}

		return _GetWatchpointValueAt(proxy->GetWatchpoint(), rowIndex,
			columnIndex, value);
	}

	BreakpointProxy* BreakpointProxyAt(int32 index) const
	{
		return fBreakpointProxies.ItemAt(index);
	}

private:

	bool _GetBreakpointValueAt(UserBreakpoint* breakpoint, int32 rowIndex,
		int32 columnIndex, BVariant &value)
	{
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
			{
				LocatableFile* sourceFile = location.SourceFile();
				if (sourceFile != NULL) {
					BString data;
					data.SetToFormat("%s:%" B_PRId32, sourceFile->Name(),
						location.GetSourceLocation().Line() + 1);
					value.SetTo(data);
				} else {
					AutoLocker<Team> teamLocker(fTeam);
					if (UserBreakpointInstance* instance
							= breakpoint->InstanceAt(0)) {
						value.SetTo(instance->Address());
					}
				}
				return true;
			}
			default:
				return false;
		}
	}

	bool _GetWatchpointValueAt(Watchpoint* watchpoint, int32 rowIndex,
		int32 columnIndex, BVariant &value)
	{
		switch (columnIndex) {
			case 0:
				value.SetTo((int32)watchpoint->IsEnabled());
				return true;
			case 1:
			{
				BString data;
				data.SetToFormat("%s at 0x%" B_PRIx64 " (%" B_PRId32 " bytes)",
					_WatchpointTypeToString(watchpoint->Type()),
					watchpoint->Address(), watchpoint->Length());
				value.SetTo(data);
				return true;
			}
			case 2:
			{
				return false;
			}
			default:
				return false;
		}
	}

	const char* _WatchpointTypeToString(uint32 type) const
	{
		switch (type) {
			case WATCHPOINT_CAPABILITY_FLAG_READ:
			{
				return "read";
			}
			case WATCHPOINT_CAPABILITY_FLAG_WRITE:
			{
				return "write";
			}
			case WATCHPOINT_CAPABILITY_FLAG_READ_WRITE:
			{
				return "read/write";
			}
			default:
				return NULL;
		}
	}

private:
	Team*						fTeam;
	BreakpointProxyList			fBreakpointProxies;
};


// #pragma mark - BreakpointListView


BreakpointListView::BreakpointListView(Team* team, Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
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
BreakpointListView::Create(Team* team, Listener* listener, BView* filterTarget)
{
	BreakpointListView* self = new BreakpointListView(team, listener);

	try {
		self->_Init(filterTarget);
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
BreakpointListView::UserBreakpointChanged(UserBreakpoint* breakpoint)
{
	BreakpointProxy proxy(breakpoint, NULL);
	fBreakpointsTableModel->UpdateBreakpoint(&proxy);
}


void
BreakpointListView::WatchpointChanged(Watchpoint* watchpoint)
{
	BreakpointProxy proxy(NULL, watchpoint);
	fBreakpointsTableModel->UpdateBreakpoint(&proxy);
}


void
BreakpointListView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("breakpointsTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fBreakpointsTable);
	}
}


status_t
BreakpointListView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
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
	BreakpointProxyList proxyList;
	for (int32 i = 0; i < selectionModel->CountRows(); i++) {
		BreakpointProxy* proxy = fBreakpointsTableModel->BreakpointProxyAt(
			selectionModel->RowAt(i));
		if (proxy == NULL)
			continue;
		if (!proxyList.AddItem(proxy))
			return;
	}

	fListener->BreakpointSelectionChanged(proxyList);
}


void
BreakpointListView::_Init(BView* filterTarget)
{
	fBreakpointsTable = new Table("breakpoints list", 0, B_FANCY_BORDER);
	AddChild(fBreakpointsTable->ToView());

	// columns
	fBreakpointsTable->AddColumn(new BoolStringTableColumn(0, "State", 70, 20,
		1000, "Enabled", "Disabled"));
	fBreakpointsTable->AddColumn(new StringTableColumn(1, "Location", 250, 40,
		1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fBreakpointsTable->AddColumn(new StringTableColumn(2, "File:Line/Address",
		250, 40, 1000, B_TRUNCATE_END, B_ALIGN_LEFT));

	fBreakpointsTable->SetSelectionMode(B_MULTIPLE_SELECTION_LIST);
	fBreakpointsTable->AddTableListener(this);
	fBreakpointsTable->AddFilter(new ListInputFilter(filterTarget));


	fBreakpointsTableModel = new BreakpointsTableModel(fTeam);
	fBreakpointsTable->SetTableModel(fBreakpointsTableModel);
}


// #pragma mark - Listener


BreakpointListView::Listener::~Listener()
{
}
