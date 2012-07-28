/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ThreadListView.h"

#include <new>

#include <Looper.h>
#include <Message.h>

#include <AutoLocker.h>
#include <ObjectList.h>
#include <ToolTip.h>

#include "GuiSettingsUtils.h"
#include "table/TableColumns.h"
#include "UiUtils.h"


enum {
	MSG_SYNC_THREAD_LIST	= 'sytl'
};


// #pragma mark - ThreadsTableModel


class ThreadListView::ThreadsTableModel : public TableModel,
	public TableToolTipProvider {
public:
	ThreadsTableModel(Team* team)
		:
		fTeam(team)
	{
		Update(-1);
	}

	~ThreadsTableModel()
	{
		fTeam = NULL;
		Update(-1);
	}

	bool Update(thread_id threadID)
	{
		if (fTeam == NULL) {
			for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++)
				thread->ReleaseReference();
			fThreads.MakeEmpty();

			return true;
		}

		AutoLocker<Team> locker(fTeam);

		ThreadList::ConstIterator it = fTeam->Threads().GetIterator();
		Thread* newThread = it.Next();
		int32 index = 0;

		// remove no longer existing threads
		while (Thread* oldThread = fThreads.ItemAt(index)) {
			if (oldThread == newThread) {
				if (threadID >= 0 && oldThread->ID() == threadID)
					NotifyRowsChanged(index, 1);
				index++;
				newThread = it.Next();
			} else {
				// TODO: Not particularly efficient!
				fThreads.RemoveItemAt(index);
				oldThread->ReleaseReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new threads
		int32 countBefore = fThreads.CountItems();
		while (newThread != NULL) {
			if (!fThreads.AddItem(newThread))
				return false;

			newThread->AcquireReference();
			newThread = it.Next();
		}

		int32 count = fThreads.CountItems();
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
		return fThreads.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		Thread* thread = fThreads.ItemAt(rowIndex);
		if (thread == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(thread->ID());
				return true;
			case 1:
			{
				const char* string = UiUtils::ThreadStateToString(
					thread->State(), thread->StoppedReason());
				value.SetTo(string, B_VARIANT_DONT_COPY_DATA);
				return true;
			}
			case 2:
				value.SetTo(thread->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			default:
				return false;
		}
	}

	virtual bool GetToolTipForTableCell(int32 rowIndex, int32 columnIndex,
		BToolTip** _tip)
	{
		Thread* thread = fThreads.ItemAt(rowIndex);
		if (thread == NULL)
			return false;

		BString text;
		text << "Thread: \"" << thread->Name() << "\" (" << thread->ID()
			<< ")\n";

		switch (thread->State()) {
			case THREAD_STATE_RUNNING:
				text << "Running";
				break;
			case THREAD_STATE_STOPPED:
			{
				switch (thread->StoppedReason()) {
					case THREAD_STOPPED_DEBUGGER_CALL:
						text << "Called debugger(): "
							<< thread->StoppedReasonInfo();
						break;
					case THREAD_STOPPED_EXCEPTION:
						text << "Caused exception: "
							<< thread->StoppedReasonInfo();
						break;
					case THREAD_STOPPED_BREAKPOINT:
					case THREAD_STOPPED_WATCHPOINT:
					case THREAD_STOPPED_SINGLE_STEP:
					case THREAD_STOPPED_DEBUGGED:
					case THREAD_STOPPED_UNKNOWN:
					default:
						text << "Stopped for debugging";
						break;
				}
				break;
			}
			case THREAD_STATE_UNKNOWN:
			default:
				text << "Current State Unknown";
				break;
		}

		BTextToolTip* tip = new(std::nothrow) BTextToolTip(text);
		if (tip == NULL)
			return false;

		*_tip = tip;
		return true;
	}

	Thread* ThreadAt(int32 index) const
	{
		return fThreads.ItemAt(index);
	}

private:
	Team*				fTeam;
	BObjectList<Thread>	fThreads;
};


// #pragma mark - ThreadListView


ThreadListView::ThreadListView(Team* team, Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fTeam(team),
	fThread(NULL),
	fThreadsTable(NULL),
	fThreadsTableModel(NULL),
	fListener(listener)
{
	SetName("Threads");
}


ThreadListView::~ThreadListView()
{
	fTeam->RemoveListener(this);
	fThreadsTable->SetTableModel(NULL);
	delete fThreadsTableModel;
}


/*static*/ ThreadListView*
ThreadListView::Create(Team* team, Listener* listener)
{
	ThreadListView* self = new ThreadListView(team, listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ThreadListView::UnsetListener()
{
	fListener = NULL;
}


void
ThreadListView::SetThread(Thread* thread)
{
	if (thread == fThread)
		return;

	if (fThread != NULL)
		fThread->ReleaseReference();

	fThread = thread;

	if (fThread != NULL) {
		fThread->AcquireReference();

		for (int32 i = 0; Thread* other = fThreadsTableModel->ThreadAt(i);
				i++) {
			if (fThread == other) {
				fThreadsTable->SelectRow(i, false);
				return;
			}
		}
	}

	fThreadsTable->DeselectAllRows();
}


void
ThreadListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SYNC_THREAD_LIST:
		{
			thread_id threadID;
			if (message->FindInt32("thread", &threadID) != B_OK)
				threadID = -1;

			fThreadsTableModel->Update(threadID);
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ThreadListView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("threadsTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fThreadsTable);
	}
}


status_t
ThreadListView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
		fThreadsTable);
	if (result == B_OK)
		result = settings.AddMessage("threadsTable", &tableSettings);

	return result;
}




void
ThreadListView::ThreadAdded(const Team::ThreadEvent& event)
{
	Looper()->PostMessage(MSG_SYNC_THREAD_LIST, this);
}


void
ThreadListView::ThreadRemoved(const Team::ThreadEvent& event)
{
	Looper()->PostMessage(MSG_SYNC_THREAD_LIST, this);
}


void
ThreadListView::ThreadStateChanged(const Team::ThreadEvent& event)
{
	BMessage message(MSG_SYNC_THREAD_LIST);
	message.AddInt32("thread", event.GetThread()->ID());

	Looper()->PostMessage(&message, this);
}


void
ThreadListView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL)
		return;

	Thread* thread = NULL;
	TableSelectionModel* selectionModel = table->SelectionModel();
	thread = fThreadsTableModel->ThreadAt(selectionModel->RowAt(0));

	fListener->ThreadSelectionChanged(thread);
}


void
ThreadListView::_Init()
{
	fThreadsTable = new Table("threads list", 0, B_FANCY_BORDER);
	AddChild(fThreadsTable->ToView());

	// columns
	fThreadsTable->AddColumn(new Int32TableColumn(0, "ID", 60, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new StringTableColumn(1, "State", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fThreadsTable->AddColumn(new StringTableColumn(2, "Name", 200, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fThreadsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fThreadsTable->AddTableListener(this);

	fThreadsTableModel = new ThreadsTableModel(fTeam);
	fThreadsTable->SetTableModel(fThreadsTableModel);
	fThreadsTable->SetToolTipProvider(fThreadsTableModel);
	fTeam->AddListener(this);
}


// #pragma mark - Listener


ThreadListView::Listener::~Listener()
{
}
