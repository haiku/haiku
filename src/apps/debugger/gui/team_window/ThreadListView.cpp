/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ThreadListView.h"

#include <stdio.h>

#include <new>

#include <Looper.h>
#include <Message.h>

#include <AutoLocker.h>
#include <ObjectList.h>

#include "table/TableColumns.h"


enum {
	MSG_SYNC_THREAD_LIST	= 'sytl'
};


// #pragma mark - ThreadsTableModel


class ThreadListView::ThreadsTableModel : public TableModel {
public:
	ThreadsTableModel(Team* team)
		:
		fTeam(team)
	{
		Update();
	}

	~ThreadsTableModel()
	{
		fTeam = NULL;
		Update();
	}

	bool Update()
	{
		if (fTeam == NULL) {
			for (int32 i = 0; Thread* thread = fThreads.ItemAt(i); i++)
				thread->RemoveReference();
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
				index++;
				newThread = it.Next();
			} else {
				// TODO: Not particularly efficient!
				fThreads.RemoveItemAt(index);
				oldThread->RemoveReference();
				NotifyRowsRemoved(index, 1);
			}
		}

		// add new threads
		int32 countBefore = fThreads.CountItems();
		while (newThread != NULL) {
			if (!fThreads.AddItem(newThread))
				return false;

			newThread->AddReference();
			newThread = it.Next();
		}

		int32 count = fThreads.CountItems();
		if (count > countBefore)
			NotifyRowsAdded(countBefore, count - countBefore);

		return true;
	}

	virtual int32 CountColumns() const
	{
		return 2;
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
				value.SetTo(thread->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			default:
				return false;
		}
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
			if (fThreadsTableModel != NULL)
				fThreadsTableModel->Update();
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
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
ThreadListView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL)
		return;

	Thread* thread = NULL;
	if (fThreadsTableModel != NULL) {
		TableSelectionModel* selectionModel = table->SelectionModel();
		thread = fThreadsTableModel->ThreadAt(selectionModel->RowAt(0));
	}

	fListener->ThreadSelectionChanged(thread);
}


void
ThreadListView::_Init()
{
	fThreadsTable = new Table("threads list", 0, B_FANCY_BORDER);
	AddChild(fThreadsTable->ToView());

	// columns
	fThreadsTable->AddColumn(new Int32TableColumn(0, "ID", 40, 20, 1000,
		B_TRUNCATE_MIDDLE, B_ALIGN_RIGHT));
	fThreadsTable->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fThreadsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fThreadsTable->AddTableListener(this);

	fThreadsTableModel = new(std::nothrow) ThreadsTableModel(fTeam);
	fThreadsTable->SetTableModel(fThreadsTableModel);
	fThreadsTable->ResizeAllColumnsToPreferred();
	fTeam->AddListener(this);
}


// #pragma mark - Listener


ThreadListView::Listener::~Listener()
{
}
