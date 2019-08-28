/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_WAIT_OBJECTS_PAGE_H
#define ABSTRACT_WAIT_OBJECTS_PAGE_H


#include <stdio.h>

#include <new>

#include <CheckBox.h>
#include <GroupView.h>

#include "model/Model.h"
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

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				AttachedToWindow();

protected:
			class WaitObjectsTreeModel;

			enum {
				MSG_ABSTRACT_WAIT_OBJECTS_GROUP_BY_NAME		= 'awog'
			};

protected:
			void				_UpdateTreeModel();

protected:
			TreeTable*			fWaitObjectsTree;
			WaitObjectsTreeModel* fWaitObjectsTreeModel;
			ModelType*			fModel;
			BCheckBox*			fGroupByNameCheckBox;
			bool				fGroupByName;


// #pragma mark - WaitObjectsTreeModel (inner class)


class WaitObjectsTreeModel : public TreeTableModel {
public:
	WaitObjectsTreeModel(ModelType* model, bool groupByName)
		:
		fModel(model),
		fRootNode(NULL)
	{
		fRootNode = new RootNode(fModel, groupByName);
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

		virtual const char* Name() const = 0;
		virtual uint32 Type() const = 0;
		virtual addr_t Object() const = 0;
		virtual addr_t ReferencedObject() const = 0;
		virtual int64 Waits() const = 0;
		virtual nanotime_t TotalWaitTime() const = 0;

		virtual int32 CountChildren() const = 0;
		virtual void* ChildAt(int32 index) const = 0;

		static int CompareByName(const Node* a, const Node* b)
		{
			int cmp = (int)a->Type() - (int)b->Type();
			return cmp == 0 ? strcmp(a->Name(), b->Name()) : cmp;
		}

		bool GetValueAt(int32 columnIndex, BVariant& value)
		{
			switch (columnIndex) {
				case 0:
					value.SetTo(wait_object_type_name(Type()),
						B_VARIANT_DONT_COPY_DATA);
					return true;
				case 1:
					value.SetTo(Name(), B_VARIANT_DONT_COPY_DATA);
					return true;
				case 2:
				{
					addr_t object = Object();
					if (object == 0)
						return false;

					char buffer[16];
					snprintf(buffer, sizeof(buffer), "%#lx", object);
					value.SetTo(buffer);
					return true;
				}
				case 3:
				{
					addr_t object = ReferencedObject();
					if (object == 0)
						return false;

					char buffer[16];
					snprintf(buffer, sizeof(buffer), "%#lx", object);
					value.SetTo(buffer);
					return true;
				}
				case 4:
					value.SetTo(Waits());
					return true;
				case 5:
					value.SetTo(TotalWaitTime());
					return true;
				default:
					return false;
			}
		}
	};

	struct ObjectNode : Node {
		WaitObjectType* object;

		ObjectNode(WaitObjectType* object)
			:
			object(object)
		{
		}

		virtual const char* Name() const
		{
			return object->Name();
		}

		virtual uint32 Type() const
		{
			return object->Type();
		}

		virtual addr_t Object() const
		{
			return object->Object();
		}

		virtual addr_t ReferencedObject() const
		{
			return object->ReferencedObject();
		}

		virtual int64 Waits() const
		{
			return object->Waits();
		}

		virtual nanotime_t TotalWaitTime() const
		{
			return object->TotalWaitTime();
		}

		virtual int32 CountChildren() const
		{
			return 0;
		}

		virtual void* ChildAt(int32 index) const
		{
			return NULL;
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

		virtual const char* Name() const
		{
			return group->Name();
		}

		virtual uint32 Type() const
		{
			return group->Type();
		}

		virtual addr_t Object() const
		{
			return group->Object();
		}

		virtual addr_t ReferencedObject() const
		{
			return 0;
		}

		virtual int64 Waits() const
		{
			return group->Waits();
		}

		virtual nanotime_t TotalWaitTime() const
		{
			return group->TotalWaitTime();
		}

		virtual int32 CountChildren() const
		{
			return objectNodes.CountItems();
		}

		virtual void* ChildAt(int32 index) const
		{
			return objectNodes.ItemAt(index);
		}
	};

	// For GCC 2
	friend struct GroupNode;

	struct NodeContainerNode : Node {
		NodeContainerNode()
			:
			fChildren(20, true)
		{
		}

		NodeContainerNode(BObjectList<Node>& nodes)
			:
			fChildren(20, true)
		{
			fChildren.AddList(&nodes);

			// compute total waits and total wait time
			fWaits = 0;
			fTotalWaitTime = 0;

			for (int32 i = 0; Node* node = fChildren.ItemAt(i); i++) {
				fWaits += node->Waits();
				fTotalWaitTime += node->TotalWaitTime();
			}
		}

		virtual const char* Name() const
		{
			Node* child = fChildren.ItemAt(0);
			return child != NULL ? child->Name() : "";
		}

		virtual uint32 Type() const
		{
			Node* child = fChildren.ItemAt(0);
			return child != NULL ? child->Type() : 0;
		}

		virtual addr_t Object() const
		{
			return 0;
		}

		virtual addr_t ReferencedObject() const
		{
			return 0;
		}

		virtual int64 Waits() const
		{
			return fWaits;
		}

		virtual nanotime_t TotalWaitTime() const
		{
			return fTotalWaitTime;
		}

		virtual int32 CountChildren() const
		{
			return fChildren.CountItems();
		}

		virtual void* ChildAt(int32 index) const
		{
			return fChildren.ItemAt(index);
		}

	protected:
		BObjectList<Node>	fChildren;
		int64				fWaits;
		nanotime_t			fTotalWaitTime;
	};

	struct RootNode : public NodeContainerNode {
		ModelType*			model;

		RootNode(ModelType* model, bool groupByName)
			:
			model(model)
		{
			// create nodes for the wait object groups
			BObjectList<Node> tempChildren;
			BObjectList<Node>& children
				= groupByName ? tempChildren : NodeContainerNode::fChildren;
			int32 count = model->CountWaitObjectGroups();
			for (int32 i = 0; i < count; i++) {
				WaitObjectGroupType* group = model->WaitObjectGroupAt(i);
				if (!children.AddItem(_CreateGroupNode(group)))
					throw std::bad_alloc();
			}

			// If we shall group the nodes by name, we create grouping nodes.
			if (groupByName) {
				if (children.CountItems() < 2) {
					NodeContainerNode::fChildren.AddList(&children);
					return;
				}

				// sort the nodes by name
				children.SortItems(&Node::CompareByName);

				// create groups
				int32 nodeCount = children.CountItems();
				BObjectList<Node> nameGroup;
				Node* previousNode = children.ItemAt(0);
				int32 groupNodeIndex = 0;
				for (int32 i = 1; i < nodeCount; i++) {
					Node* node = children.ItemAt(i);
					if (strcmp(node->Name(), previousNode->Name())) {
						// create the group -- or just add the node, if it's
						// the only one
						if (nameGroup.CountItems() > 1) {
							NodeContainerNode::fChildren.AddItem(
								new NodeContainerNode(nameGroup));
						} else
							NodeContainerNode::fChildren.AddItem(previousNode);

						nameGroup.MakeEmpty();
						groupNodeIndex = i;
					}

					// add the node
					nameGroup.AddItem(node);

					previousNode = node;
				}
			}
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
	ModelType*	fModel;
	RootNode*	fRootNode;
};	// WaitObjectsTreeModel

};	// AbstractWaitObjectsPage


// #pragma mark - WaitObjectsPage


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::AbstractWaitObjectsPage()
	:
	BGroupView(B_VERTICAL, 10),
	fWaitObjectsTree(NULL),
	fWaitObjectsTreeModel(NULL),
	fModel(NULL),
	fGroupByNameCheckBox(NULL),
	fGroupByName(false)
{
	SetName("Wait objects");

	fWaitObjectsTree = new TreeTable("wait object list", 0);
	AddChild(fWaitObjectsTree->ToView());

	fGroupByNameCheckBox = new BCheckBox("group by name checkbox",
		"Group by name", new BMessage(MSG_ABSTRACT_WAIT_OBJECTS_GROUP_BY_NAME));
	fGroupByNameCheckBox->SetValue(
		fGroupByName ? B_CONTROL_ON : B_CONTROL_OFF);
	AddChild(fGroupByNameCheckBox);

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
	fWaitObjectsTree->AddColumn(new NanotimeTableColumn(5, "Wait time", 80,
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

	if (fModel != NULL)
		_UpdateTreeModel();
}


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
void
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_ABSTRACT_WAIT_OBJECTS_GROUP_BY_NAME:
			fGroupByName = fGroupByNameCheckBox->Value() == B_CONTROL_ON;
			_UpdateTreeModel();
			break;
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
void
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::AttachedToWindow()
{
	fGroupByNameCheckBox->SetTarget(this);
}


ABSTRACT_WAIT_OBJECTS_PAGE_TEMPLATE
void
ABSTRACT_WAIT_OBJECTS_PAGE_CLASS::_UpdateTreeModel()
{
	if (fModel != NULL) {
		fWaitObjectsTree->SetTreeTableModel(NULL);
		delete fWaitObjectsTreeModel;
		fWaitObjectsTreeModel = NULL;
	}

	if (fModel != NULL) {
		try {
			fWaitObjectsTreeModel = new WaitObjectsTreeModel(fModel,
				fGroupByName);
		} catch (std::bad_alloc&) {
			// TODO: Report error!
		}
		fWaitObjectsTree->SetTreeTableModel(fWaitObjectsTreeModel);
		fWaitObjectsTree->ResizeAllColumnsToPreferred();
	}
}


#endif	// ABSTRACT_WAIT_OBJECTS_PAGE_H
