/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "thread_window/WaitObjectsPage.h"

#include <stdio.h>

#include <new>

#include <thread_defs.h>

#include "table/TableColumns.h"


// #pragma mark - WaitObjectsTreeModel


class ThreadWindow::WaitObjectsPage::WaitObjectsTreeModel
	: public TreeTableModel {
public:
	WaitObjectsTreeModel(Model* model, Model::Thread* thread)
		:
		fModel(model),
		fThread(thread),
		fRootNode(NULL)
	{
		fRootNode = new RootNode(thread);
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
		return 14;
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
			return _GetWaitObjectValueAt(object->GetWaitObject(), columnIndex,
				value);
		}
	};

	struct GroupNode : Node {
		Model::ThreadWaitObjectGroup*	group;
		BObjectList<ObjectNode>			objectNodes;

		GroupNode(Model::ThreadWaitObjectGroup* group)
			:
			group(group)
		{
			BObjectList<Model::ThreadWaitObject> objects;
			if (!group->GetThreadWaitObjects(objects))
				throw std::bad_alloc();

			int32 count = objects.CountItems();
			for (int32 i = 0; i < count; i++) {
				if (!objectNodes.AddItem(new ObjectNode(objects.ItemAt(i))))
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
			if (columnIndex >= 3)
				return false;

			return _GetWaitObjectValueAt(group->MostRecentWaitObject(),
				columnIndex, value);
		}
	};

	struct RootNode : Node {
		Model::Thread*			thread;
		BObjectList<GroupNode>	groupNodes;

		RootNode(Model::Thread* thread)
			:
			thread(thread),
			groupNodes(20, true)
		{
			int32 count = thread->CountThreadWaitObjectGroups();
			for (int32 i = 0; i < count; i++) {
				Model::ThreadWaitObjectGroup* group
					= thread->ThreadWaitObjectGroupAt(i);
				if (!groupNodes.AddItem(new GroupNode(group)))
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

	static bool _GetWaitObjectValueAt(Model::WaitObject* waitObject,
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
			default:
				return false;
		}
	}

private:
	Model*			fModel;
	Model::Thread*	fThread;
	RootNode*		fRootNode;
};


// #pragma mark - WaitObjectsPage


ThreadWindow::WaitObjectsPage::WaitObjectsPage()
	:
	BGroupView(B_VERTICAL),
	fWaitObjectsTree(NULL),
	fWaitObjectsTreeModel(NULL),
	fModel(NULL)

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

	fWaitObjectsTree->AddTreeTableListener(this);
}


ThreadWindow::WaitObjectsPage::~WaitObjectsPage()
{
	fWaitObjectsTree->SetTreeTableModel(NULL);
	delete fWaitObjectsTreeModel;
}


void
ThreadWindow::WaitObjectsPage::SetModel(Model* model, Model::Thread* thread)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fWaitObjectsTree->SetTreeTableModel(NULL);
		delete fWaitObjectsTreeModel;
		fWaitObjectsTreeModel = NULL;
	}

	fModel = model;
	fThread = thread;

	if (fModel != NULL) {
		try {
			fWaitObjectsTreeModel
				= new WaitObjectsTreeModel(fModel, fThread);
		} catch (std::bad_alloc) {
			// TODO: Report error!
		}
		fWaitObjectsTree->SetTreeTableModel(fWaitObjectsTreeModel);
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
