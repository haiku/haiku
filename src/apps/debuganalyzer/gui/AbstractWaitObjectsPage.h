/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_WAIT_OBJECTS_PAGE_H
#define ABSTRACT_WAIT_OBJECTS_PAGE_H


#include <stdio.h>

#include <new>

#include <GroupView.h>

#include "table/TableColumns.h"
#include "table/TreeTable.h"


#define ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE						\
	template<typename ModelType, typename WaitObjectGroupType,	\
		typename WaitObjectType>
#define ABSTRACT_WAIT_OBJECTS_PAGE_CLASS	\
	AbstractWaitObjectsPage<ModelType, WaitObjectGroupType,	WaitObjectType>


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
class AbstractWaitObjectsPage : public BGroupView, protected TreeTableListener {
public:
								AbstractWaitObjectsPage();
	virtual						~AbstractWaitObjectsPage();

			void				SetModel(ModelType* model);

protected:
			class WaitObjectsTreeModel;

protected:
			TreeTable*			fWaitObjectsTree;
			WaitObjectsTreeModel* fWaitObjectsTreeModel;
			ModelType*			fModel;
};


// #pragma mark - WaitObjectsTreeModel


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
class ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::WaitObjectsTreeModel
	: public TreeTableModel {
public:
	WaitObjectsTreeModel(ModelType* model)
		:
		fModel(model),
		fRootNode(NULL)
	{
		fRootNode = new RootNode(fModel);
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

	virtual bool GetValueAt(void* object, int32 columnIndex, BVariant& value)
	{
		return ((Node*)object)->GetValueAt(columnIndex, value);
	}

private:
	struct Node {
		virtual ~Node() {}

		virtual int32 CountChildren() const = 0;
		virtual void* ChildAt(int32 index) const = 0;
		virtual bool GetValueAt(int32 columnIndex, BVariant& value) = 0;
	};

	struct ObjectNode : Node {
		WaitObjectType* object;

		ObjectNode(WaitObjectType* object)
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

		virtual bool GetValueAt(int32 columnIndex, BVariant& value)
		{
			return _GetWaitObjectValueAt(object, columnIndex, value);
		}
	};

	// For GCC 2
	friend struct ObjectNode;

	struct GroupNode : Node {
		WaitObjectGroupType*	group;
		BObjectList<ObjectNode>	objectNodes;

		GroupNode(WaitObjectGroupType* group)
			:
			group(group)
		{
			int32 count = group->CountWaitObjects();
			for (int32 i = 0; i < count; i++) {
				WaitObjectType* waitObject = group->WaitObjectAt(i);
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

		virtual bool GetValueAt(int32 columnIndex, BVariant& value)
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
		ModelType*			model;
		BObjectList<Node>	groupNodes;

		RootNode(ModelType* model)
			:
			model(model),
			groupNodes(20, true)
		{
			int32 count = model->CountWaitObjectGroups();
			for (int32 i = 0; i < count; i++) {
				WaitObjectGroupType* group = model->WaitObjectGroupAt(i);
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

		virtual bool GetValueAt(int32 columnIndex, BVariant& value)
		{
			return false;
		}

	private:
		static Node* _CreateGroupNode(WaitObjectGroupType* group)
		{
			// If the group has only one object, just create an object node.
			if (group->CountWaitObjects() == 1)
				return new ObjectNode(group->WaitObjectAt(0));

			return new GroupNode(group);
		}

	};

private:
	static bool _GetWaitObjectValueAt(WaitObjectType* waitObject,
		int32 columnIndex, BVariant& value)
	{
		switch (columnIndex) {
			case 0:
				value.SetTo(wait_object_type_name(waitObject->Type()),
					B_VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				value.SetTo(waitObject->Name(), B_VARIANT_DONT_COPY_DATA);
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
	ModelType*	fModel;
	RootNode*	fRootNode;
};


// #pragma mark - WaitObjectsPage


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::AbstractWaitObjectsPage()
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
	fWaitObjectsTree->AddColumn(new Int64TableColumn(4, "Waits", 80, 20,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fWaitObjectsTree->AddColumn(new NanotimeTableColumn(5, "Wait Time", 80,
		20, 1000, false, B_TRUNCATE_END, B_ALIGN_RIGHT));

	fWaitObjectsTree->AddTreeTableListener(this);
}


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::~AbstractWaitObjectsPage()
{
	fWaitObjectsTree->SetTreeTableModel(NULL);
	delete fWaitObjectsTreeModel;
}


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
void
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::SetModel(ModelType* model)
{
	if (model == fModel)
		return;

	if (fModel != NULL) {
		fWaitObjectsTree->SetTreeTableModel(NULL);
		delete fWaitObjectsTreeModel;
		fWaitObjectsTreeModel = NULL;
	}

	fModel = model;

	if (fModel != NULL) {
		try {
			fWaitObjectsTreeModel = new WaitObjectsTreeModel(fModel);
		} catch (std::bad_alloc) {
			// TODO: Report error!
		}
		fWaitObjectsTree->SetTreeTableModel(fWaitObjectsTreeModel);
		fWaitObjectsTree->ResizeAllColumnsToPreferred();
	}
}


#endif	// ABSTRACT_WAIT_OBJECTS_PAGE_H
