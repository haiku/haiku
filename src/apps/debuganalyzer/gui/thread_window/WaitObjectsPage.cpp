/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "thread_window/WaitObjectsPage.h"

#include <stdio.h>

#include <new>

#include <thread_defs.h>

#include "ThreadModel.h"

#include "table/TableColumns.h"


// #pragma mark - WaitObjectsTreeModel


class ThreadWindow::WaitObjectsPage::WaitObjectsTreeModel
	: public TreeTableModel {
public:
	WaitObjectsTreeModel(ThreadModel* model)
		:
		fThreadModel(model),
		fRootNode(NULL)
	{
		fRootNode = new RootNode(fThreadModel);
	}

	~WaitObjectsTreeModel()
	{
		delete fRootNode;
	}

	virtual void* Root() const
	{
		return fRootNode;
	}

	virtual int32 CountChildren(void* parent) const
	{
		return ((Node*)parent)->CountChildren();
	}

	virtual void* ChildAt(void* parent, int32 index) const
	{
		return ((Node*)parent)->ChildAt(index);
	}

	virtual int32 CountColumns() const
	{
		return 6;
	}

	virtual bool GetValueAt(void* object, int32 columnIndex, Variant& value)
	{
		return ((Node*)object)->GetValueAt(columnIndex, value);
	}

private:
	struct Node {
		virtual ~Node() {}

		virtual int32 CountChildren() const = 0;
		virtual void* ChildAt(int32 index) const = 0;
		virtual bool GetValueAt(int32 columnIndex, Variant& value) = 0;
	};

	struct ObjectNode : Node {
		Model::ThreadWaitObject* object;

		ObjectNode(Model::ThreadWaitObject* object)
			:
			object(object)
		{
		}

		virtual int32 CountChildren() const
		{
			return 0;
		}

		virtual void* ChildAt(int32 index) const
		{
			return NULL;
		}

		virtual bool GetValueAt(int32 columnIndex, Variant& value)
		{
			return _GetWaitObjectValueAt(object, columnIndex, value);
		}
	};

	// For GCC 2
	friend struct ObjectNode;

	struct GroupNode : Node {
		ThreadModel::WaitObjectGroup*	group;
		BObjectList<ObjectNode>			objectNodes;

		GroupNode(ThreadModel::WaitObjectGroup* group)
			:
			group(group)
		{
			int32 count = group->CountWaitObjects();
			for (int32 i = 0; i < count; i++) {
				Model::ThreadWaitObject* waitObject = group->WaitObjectAt(i);
				if (!objectNodes.AddItem(new ObjectNode(waitObject)))
					throw std::bad_alloc();
			}
		}

		virtual int32 CountChildren() const
		{
			return objectNodes.CountItems();
		}

		virtual void* ChildAt(int32 index) const
		{
			return objectNodes.ItemAt(index);
		}

		virtual bool GetValueAt(int32 columnIndex, Variant& value)
		{
			if (columnIndex <= 2) {
				return _GetWaitObjectValueAt(group->WaitObjectAt(0),
					columnIndex, value);
			}

			switch (columnIndex) {
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
	};

	// For GCC 2
	friend struct GroupNode;

	struct RootNode : Node {
		ThreadModel*		threadModel;
		BObjectList<Node>	groupNodes;

		RootNode(ThreadModel* model)
			:
			threadModel(model),
			groupNodes(20, true)
		{
			int32 count = threadModel->CountWaitObjectGroups();
			for (int32 i = 0; i < count; i++) {
				ThreadModel::WaitObjectGroup* group
					= threadModel->WaitObjectGroupAt(i);
				if (!groupNodes.AddItem(_CreateGroupNode(group)))
					throw std::bad_alloc();
			}
		}

		virtual int32 CountChildren() const
		{
			return groupNodes.CountItems();
		}

		virtual void* ChildAt(int32 index) const
		{
			return groupNodes.ItemAt(index);
		}

		virtual bool GetValueAt(int32 columnIndex, Variant& value)
		{
			return false;
		}

	private:
		static Node* _CreateGroupNode(ThreadModel::WaitObjectGroup* group)
		{
			// If the group has only one object, just create an object node.
			if (group->CountWaitObjects() == 1)
				return new ObjectNode(group->WaitObjectAt(0));

			return new GroupNode(group);
		}

	};

private:
	static const char* _TypeToString(uint32 type)
	{
		switch (type) {
			case THREAD_BLOCK_TYPE_SEMAPHORE:
				return "semaphore";
			case THREAD_BLOCK_TYPE_CONDITION_VARIABLE:
				return "condition";
			case THREAD_BLOCK_TYPE_MUTEX:
				return "mutex";
			case THREAD_BLOCK_TYPE_RW_LOCK:
				return "rw lock";
			case THREAD_BLOCK_TYPE_OTHER:
				return "other";
			case THREAD_BLOCK_TYPE_SNOOZE:
				return "snooze";
			case THREAD_BLOCK_TYPE_SIGNAL:
				return "signal";
			default:
				return "unknown";
		}
	}

	static bool _GetWaitObjectValueAt(Model::ThreadWaitObject* waitObject,
		int32 columnIndex, Variant& value)
	{
		switch (columnIndex) {
			case 0:
				value.SetTo(_TypeToString(waitObject->Type()),
					VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				value.SetTo(waitObject->Name(), VARIANT_DONT_COPY_DATA);
				return true;
			case 2:
			{
				if (waitObject->Object() == 0)
					return false;

				char buffer[16];
				snprintf(buffer, sizeof(buffer), "%#lx",
					waitObject->Object());
				value.SetTo(buffer);
				return true;
			}
			case 3:
			{
				if (waitObject->ReferencedObject() == 0)
					return false;

				char buffer[16];
				snprintf(buffer, sizeof(buffer), "%#lx",
					waitObject->ReferencedObject());
				value.SetTo(buffer);
				return true;
			}
			case 4:
				value.SetTo(waitObject->Waits());
				return true;
			case 5:
				value.SetTo(waitObject->TotalWaitTime());
				return true;
			default:
				return false;
		}
	}

private:
	ThreadModel*	fThreadModel;
	RootNode*		fRootNode;
};


// #pragma mark - WaitObjectsPage


ThreadWindow::WaitObjectsPage::WaitObjectsPage()
	:
	BGroupView(B_VERTICAL),
	fWaitObjectsTree(NULL),
	fWaitObjectsTreeModel(NULL),
	fThreadModel(NULL)

{
	SetName("Wait Objects");

	fWaitObjectsTree = new TreeTable("wait object list", 0);
	AddChild(fWaitObjectsTree->ToView());

	fWaitObjectsTree->AddColumn(new StringTableColumn(0, "Type", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTree->AddColumn(new StringTableColumn(1, "Name", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTree->AddColumn(new StringTableColumn(2, "Object", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTree->AddColumn(new StringTableColumn(3, "Referenced", 80, 40,
		1000, B_TRUNCATE_END, B_ALIGN_LEFT));
	fWaitObjectsTree->AddColumn(new Int64TableColumn(4, "Waits", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fWaitObjectsTree->AddColumn(new BigtimeTableColumn(5, "Wait Time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));

	fWaitObjectsTree->AddTreeTableListener(this);
}


ThreadWindow::WaitObjectsPage::~WaitObjectsPage()
{
	fWaitObjectsTree->SetTreeTableModel(NULL);
	delete fWaitObjectsTreeModel;
}


void
ThreadWindow::WaitObjectsPage::SetModel(ThreadModel* model)
{
	if (model == fThreadModel)
		return;

	if (fThreadModel != NULL) {
		fWaitObjectsTree->SetTreeTableModel(NULL);
		delete fWaitObjectsTreeModel;
		fWaitObjectsTreeModel = NULL;
	}

	fThreadModel = model;

	if (fThreadModel != NULL) {
		try {
			fWaitObjectsTreeModel = new WaitObjectsTreeModel(fThreadModel);
		} catch (std::bad_alloc) {
			// TODO: Report error!
		}
		fWaitObjectsTree->SetTreeTableModel(fWaitObjectsTreeModel);
		fWaitObjectsTree->ResizeAllColumnsToPreferred();
	}
}


void
ThreadWindow::WaitObjectsPage::TreeTableNodeInvoked(TreeTable* table,
	void* node)
{
//	Model::Thread* thread = fModel->ThreadAt(rowIndex);
//	if (thread != NULL)
//		fParent->OpenThreadWindow(thread);
}
