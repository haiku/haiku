/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "table/TreeTable.h"

#include <new>


// #pragma mark - TreeTableField


class TreeTableField : public BField {
public:
	TreeTableField(void* object)
		:
		fObject(object)
	{
	}

	void* Object() const
	{
		return fObject;
	}

private:
	void*	fObject;
};


// #pragma mark - TreeTablePath


TreeTablePath::TreeTablePath()
{
}


TreeTablePath::TreeTablePath(const TreeTablePath& other)
{
	*this = other;
}


TreeTablePath::TreeTablePath(const TreeTablePath& other, int32 childIndex)
{
	*this = other;
	AddComponent(childIndex);
}


TreeTablePath::~TreeTablePath()
{
}


bool
TreeTablePath::AddComponent(int32 childIndex)
{
	try {
		fComponents.push_back(childIndex);
		return true;
	} catch (...) {
		return false;
	}
}


int32
TreeTablePath::RemoveLastComponent()
{
	if (fComponents.empty())
		return -1;

	int32 index = fComponents.back();
	fComponents.pop_back();
	return index;
}

void
TreeTablePath::Clear()
{
	fComponents.clear();
}


int32
TreeTablePath::CountComponents() const
{
	return fComponents.size();
}


int32
TreeTablePath::ComponentAt(int32 index) const
{
	if (index < 0 || (size_t)index >= fComponents.size())
		return -1;
	return fComponents[index];
}


TreeTablePath&
TreeTablePath::operator=(const TreeTablePath& other)
{
	try {
		fComponents = other.fComponents;
	} catch (...) {
	}
	return *this;
}


bool
TreeTablePath::operator==(const TreeTablePath& other) const
{
	return fComponents == other.fComponents;
}


bool
TreeTablePath::operator!=(const TreeTablePath& other) const
{
	return fComponents != other.fComponents;
}


// #pragma mark - TreeTableModelListener


TreeTableModelListener::~TreeTableModelListener()
{
}


void
TreeTableModelListener::TableNodesAdded(TreeTableModel* model,
	const TreeTablePath& path, int32 childIndex, int32 count)
{
}


void
TreeTableModelListener::TableNodesRemoved(TreeTableModel* model,
	const TreeTablePath& path, int32 childIndex, int32 count)
{
}


void
TreeTableModelListener::TableNodesChanged(TreeTableModel* model,
	const TreeTablePath& path, int32 childIndex, int32 count)
{
}


// #pragma mark - TreeTableModel


TreeTableModel::~TreeTableModel()
{
}


void*
TreeTableModel::NodeForPath(const TreeTablePath& path) const
{
	void* node = Root();

	int32 count = path.CountComponents();
	for (int32 i = 0; node != NULL && i < count; i++)
		node = ChildAt(node, path.ComponentAt(i));

	return node;
}


bool
TreeTableModel::AddListener(TreeTableModelListener* listener)
{
	return fListeners.AddItem(listener);
}


void
TreeTableModel::RemoveListener(TreeTableModelListener* listener)
{
	fListeners.RemoveItem(listener);
}


void
TreeTableModel::NotifyNodesAdded(const TreeTablePath& path, int32 childIndex,
	int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TreeTableModelListener* listener = fListeners.ItemAt(i);
		listener->TableNodesAdded(this, path, childIndex, count);
	}
}


void
TreeTableModel::NotifyNodesRemoved(const TreeTablePath& path, int32 childIndex,
	int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TreeTableModelListener* listener = fListeners.ItemAt(i);
		listener->TableNodesRemoved(this, path, childIndex, count);
	}
}


void
TreeTableModel::NotifyNodesChanged(const TreeTablePath& path, int32 childIndex,
	int32 count)
{
	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--) {
		TreeTableModelListener* listener = fListeners.ItemAt(i);
		listener->TableNodesChanged(this, path, childIndex, count);
	}
}


// #pragma mark - TreeTableToolTipProvider


TreeTableToolTipProvider::~TreeTableToolTipProvider()
{
}


// #pragma mark - TreeTableListener


TreeTableListener::~TreeTableListener()
{
}


void
TreeTableListener::TreeTableSelectionChanged(TreeTable* table)
{
}


void
TreeTableListener::TreeTableNodeInvoked(TreeTable* table,
	const TreeTablePath& path)
{
}


void
TreeTableListener::TreeTableNodeExpandedChanged(TreeTable* table,
	const TreeTablePath& path, bool expanded)
{
}


void
TreeTableListener::TreeTableCellMouseDown(TreeTable* table,
	const TreeTablePath& path, int32 columnIndex, BPoint screenWhere,
	uint32 buttons)
{
}


void
TreeTableListener::TreeTableCellMouseUp(TreeTable* table,
	const TreeTablePath& path, int32 columnIndex, BPoint screenWhere,
	uint32 buttons)
{
}


// #pragma mark - Column


class TreeTable::Column : public AbstractColumn {
public:
								Column(TreeTableModel* model,
									TableColumn* tableColumn);
	virtual						~Column();

	virtual	void				SetModel(AbstractTableModelBase* model);

protected:
	virtual	void				DrawTitle(BRect rect, BView* targetView);
	virtual	void				DrawField(BField* field, BRect rect,
									BView* targetView);
	virtual	int					CompareFields(BField* field1, BField* field2);

	virtual	void				GetColumnName(BString* into) const;
	virtual	float				GetPreferredWidth(BField* field,
									BView* parent) const;

private:
			TreeTableModel*		fModel;
};


TreeTable::Column::Column(TreeTableModel* model, TableColumn* tableColumn)
	:
	AbstractColumn(tableColumn),
	fModel(model)
{
}


TreeTable::Column::~Column()
{
}


void
TreeTable::Column::SetModel(AbstractTableModelBase* model)
{
	fModel = dynamic_cast<TreeTableModel*>(model);
}


void
TreeTable::Column::DrawTitle(BRect rect, BView* targetView)
{
	fTableColumn->DrawTitle(rect, targetView);
}


void
TreeTable::Column::DrawField(BField* _field, BRect rect, BView* targetView)
{
	TreeTableField* field = dynamic_cast<TreeTableField*>(_field);
	if (field == NULL)
		return;

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value;
	if (!fModel->GetValueAt(field->Object(), modelIndex, value))
		return;
	fTableColumn->DrawValue(value, rect, targetView);
}


int
TreeTable::Column::CompareFields(BField* _field1, BField* _field2)
{
	TreeTableField* field1 = dynamic_cast<TreeTableField*>(_field1);
	TreeTableField* field2 = dynamic_cast<TreeTableField*>(_field2);

	if (field1 == field2)
		return 0;

	if (field1 == NULL)
		return -1;
	if (field2 == NULL)
		return 1;

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value1;
	bool valid1 = fModel->GetValueAt(field1->Object(), modelIndex, value1);
	BVariant value2;
	bool valid2 = fModel->GetValueAt(field2->Object(), modelIndex, value2);

	if (!valid1)
		return valid2 ? -1 : 0;
	if (!valid2)
		return 1;

	return fTableColumn->CompareValues(value1, value2);
}


void
TreeTable::Column::GetColumnName(BString* into) const
{
	fTableColumn->GetColumnName(into);
}


float
TreeTable::Column::GetPreferredWidth(BField* _field, BView* parent) const
{
	TreeTableField* field = dynamic_cast<TreeTableField*>(_field);
	if (field == NULL)
		return Width();

	int32 modelIndex = fTableColumn->ModelIndex();
	BVariant value;
	if (!fModel->GetValueAt(field->Object(), modelIndex, value))
		return Width();
	return fTableColumn->GetPreferredWidth(value, parent);
}


// #pragma mark - TreeTableRow


class TreeTableRow : public BRow {
public:
	TreeTableRow(TreeTableNode* node)
		:
		fNode(node)
	{
	}

	TreeTableNode* Node()
	{
		return fNode;
	}

private:
	TreeTableNode*	fNode;
};


// #pragma mark - TreeTableNode


class TreeTableNode {
public:
								TreeTableNode(TreeTableNode* parent);
								~TreeTableNode();

			status_t			Init(void* modelObject, int32 columnCount);
			void				DetachRow();

			TreeTableNode*		Parent() const	{ return fParent; }
			TreeTableRow*		Row() const		{ return fRow; }
			void*				ModelObject() const;

			bool				AddChild(TreeTableNode* child, int32 index);
			TreeTableNode*		RemoveChild(int32 index);

			int32				CountChildren() const;
			TreeTableNode*		ChildAt(int32 index);
			int32				IndexOf(TreeTableNode* child);

private:
			typedef BObjectList<TreeTableNode> NodeList;

private:
			TreeTableNode*		fParent;
			TreeTableRow*		fRow;
			NodeList*			fChildren;
};


TreeTableNode::TreeTableNode(TreeTableNode* parent)
	:
	fParent(parent),
	fRow(NULL),
	fChildren(NULL)
{
}


TreeTableNode::~TreeTableNode()
{
	delete fChildren;
	delete fRow;
}


status_t
TreeTableNode::Init(void* modelObject, int32 columnCount)
{
	// create row
	fRow = new(std::nothrow) TreeTableRow(this);
	if (fRow == NULL)
		return B_NO_MEMORY;

	// add dummy fields to row
	for (int32 columnIndex = 0; columnIndex < columnCount; columnIndex++) {
		// It would be nice to create only a single field and set it for all
		// columns, but the row takes ultimate ownership, so it have to be
		// individual objects.
		TreeTableField* field = new(std::nothrow) TreeTableField(modelObject);
		if (field == NULL)
			return B_NO_MEMORY;

		fRow->SetField(field, columnIndex);
	}

	return B_OK;
}


void
TreeTableNode::DetachRow()
{

	fRow = NULL;

	if (fChildren != NULL) {
		for (int32 i = 0; TreeTableNode* child = fChildren->ItemAt(i); i++)
			child->DetachRow();
	}
}


void*
TreeTableNode::ModelObject() const
{
	TreeTableField* field = dynamic_cast<TreeTableField*>(fRow->GetField(0));
	return field->Object();
}


bool
TreeTableNode::AddChild(TreeTableNode* child, int32 index)
{
	if (fChildren == NULL) {
		fChildren = new(std::nothrow) NodeList(10, true);
		if (fChildren == NULL)
			return false;
	}

	return fChildren->AddItem(child, index);
}


TreeTableNode*
TreeTableNode::RemoveChild(int32 index)
{
	return fChildren != NULL ? fChildren->RemoveItemAt(index) : NULL;
}


int32
TreeTableNode::CountChildren() const
{
	return fChildren != NULL ? fChildren->CountItems() : 0;
}


TreeTableNode*
TreeTableNode::ChildAt(int32 index)
{
	return fChildren != NULL ? fChildren->ItemAt(index) : NULL;
}


int32
TreeTableNode::IndexOf(TreeTableNode* child)
{
	return fChildren != NULL ? fChildren->IndexOf(child) : -1;
}


// #pragma mark - TreeTableSelectionModel


TreeTableSelectionModel::TreeTableSelectionModel(TreeTable* table)
	:
	fTreeTable(table),
	fNodes(NULL),
	fNodeCount(-1)
{
}


TreeTableSelectionModel::~TreeTableSelectionModel()
{
	delete[] fNodes;
}


int32
TreeTableSelectionModel::CountNodes()
{
	_Update();

	return fNodeCount;
}


void*
TreeTableSelectionModel::NodeAt(int32 index)
{
	if (TreeTableNode* node = _NodeAt(index))
		return node->ModelObject();
	return NULL;
}


bool
TreeTableSelectionModel::GetPathAt(int32 index, TreeTablePath& _path)
{
	if (TreeTableNode* node = _NodeAt(index)) {
		fTreeTable->_GetPathForNode(node, _path);
		return true;
	}

	return false;
}


void
TreeTableSelectionModel::_SelectionChanged()
{
	if (fNodeCount >= 0) {
		fNodeCount = -1;
		delete[] fNodes;
		fNodes = NULL;
	}
}


void
TreeTableSelectionModel::_Update()
{
	if (fNodeCount >= 0)
		return;

	// count the nodes
	fNodeCount = 0;
	BRow* row = NULL;
	while ((row = fTreeTable->CurrentSelection(row)) != NULL)
		fNodeCount++;

	if (fNodeCount == 0)
		return;

	// allocate node array
	fNodes = new(std::nothrow) TreeTableNode*[fNodeCount];
	if (fNodes == NULL) {
		fNodeCount = 0;
		return;
	}

	// get the nodes
	row = NULL;
	int32 index = 0;
	while ((row = fTreeTable->CurrentSelection(row)) != NULL)
		fNodes[index++] = dynamic_cast<TreeTableRow*>(row)->Node();
}


TreeTableNode*
TreeTableSelectionModel::_NodeAt(int32 index)
{
	_Update();

	return index >= 0 && index < fNodeCount ? fNodes[index] : NULL;
}


// #pragma mark - Table


TreeTable::TreeTable(const char* name, uint32 flags, border_style borderStyle,
	bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fToolTipProvider(NULL),
	fRootNode(NULL),
	fSelectionModel(this),
	fIgnoreSelectionChange(0)
{
}


TreeTable::TreeTable(TreeTableModel* model, const char* name, uint32 flags,
	border_style borderStyle, bool showHorizontalScrollbar)
	:
	AbstractTable(name, flags, borderStyle, showHorizontalScrollbar),
	fModel(NULL),
	fRootNode(NULL),
	fSelectionModel(this),
	fIgnoreSelectionChange(0)
{
	SetTreeTableModel(model);
}


TreeTable::~TreeTable()
{
	SetTreeTableModel(NULL);

	for (int32 i = CountColumns() - 1; i >= 0; i--)
		RemoveColumn(ColumnAt(i));
}


bool
TreeTable::SetTreeTableModel(TreeTableModel* model)
{
	if (model == fModel)
		return true;

	if (fModel != NULL) {
		fModel->RemoveListener(this);

		if (fRootNode != NULL) {
			fRootNode->DetachRow();
			delete fRootNode;
			fRootNode = NULL;
		}

		Clear();

		for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
			column->SetModel(NULL);
	}

	fModel = model;

	if (fModel == NULL)
		return true;

	fRootNode = new(std::nothrow) TreeTableNode(NULL);
	if (fRootNode == NULL)
		return false;

	if (fRootNode->Init(fModel->Root(), fModel->CountColumns()) != B_OK) {
		delete fRootNode;
		fRootNode = NULL;
		return false;
	}

	fModel->AddListener(this);

	for (int32 i = 0; AbstractColumn* column = fColumns.ItemAt(i); i++)
		column->SetModel(fModel);

	// recursively create the rows
	if (!_AddChildRows(fRootNode, 0, fModel->CountChildren(fModel->Root()),
			fModel->CountColumns())) {
		SetTreeTableModel(NULL);
		return false;
	}

	return true;
}


void
TreeTable::SetToolTipProvider(TreeTableToolTipProvider* toolTipProvider)
{
	fToolTipProvider = toolTipProvider;
}


TreeTableSelectionModel*
TreeTable::SelectionModel()
{
	return &fSelectionModel;
}


void
TreeTable::SelectNode(const TreeTablePath& path, bool extendSelection)
{
	TreeTableNode* node = _NodeForPath(path);
	if (node == NULL)
		return;

	if (!extendSelection) {
		fIgnoreSelectionChange++;
		DeselectAll();
		fIgnoreSelectionChange--;
	}

	AddToSelection(node->Row());
}


void
TreeTable::DeselectNode(const TreeTablePath& path)
{
	if (TreeTableNode* node = _NodeForPath(path))
		Deselect(node->Row());
}


void
TreeTable::DeselectAllNodes()
{
	DeselectAll();
}


bool
TreeTable::IsNodeExpanded(const TreeTablePath& path) const
{
	if (TreeTableNode* node = _NodeForPath(path))
		return node->Row()->IsExpanded();
	return false;
}


void
TreeTable::SetNodeExpanded(const TreeTablePath& path, bool expanded,
	bool expandAncestors)
{
	if (TreeTableNode* node = _NodeForPath(path))
		_SetNodeExpanded(node, expanded, expandAncestors);
}


void
TreeTable::ScrollToNode(const TreeTablePath& path)
{
	if (TreeTableNode* node = _NodeForPath(path))
		BColumnListView::ScrollTo(node->Row());
}


bool
TreeTable::AddTreeTableListener(TreeTableListener* listener)
{
	return fListeners.AddItem(listener);
}


void
TreeTable::RemoveTreeTableListener(TreeTableListener* listener)
{
	fListeners.RemoveItem(listener);
}


bool
TreeTable::GetToolTipAt(BPoint point, BToolTip** _tip)
{
	if (fToolTipProvider == NULL)
		return AbstractTable::GetToolTipAt(point, _tip);

	// get the table row
	BRow* row = RowAt(point);
	if (row == NULL)
		return AbstractTable::GetToolTipAt(point, _tip);

	TreeTableRow* treeRow = dynamic_cast<TreeTableRow*>(row);
	// get the table column
	BColumn* column = ColumnAt(point);

	int32 columnIndex = column != NULL ? column->LogicalFieldNum() : -1;

	TreeTablePath path;
	_GetPathForNode(treeRow->Node(), path);

	return fToolTipProvider->GetToolTipForTablePath(path, columnIndex,
		_tip);
}


void
TreeTable::SelectionChanged()
{
	if (fIgnoreSelectionChange > 0)
		return;

	fSelectionModel._SelectionChanged();

	if (!fListeners.IsEmpty()) {
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--)
			fListeners.ItemAt(i)->TreeTableSelectionChanged(this);
	}
}


AbstractTable::AbstractColumn*
TreeTable::CreateColumn(TableColumn* column)
{
	return new Column(fModel, column);
}


void
TreeTable::ColumnMouseDown(AbstractColumn* column, BRow* _row, BField* field,
	BPoint screenWhere, uint32 buttons)
{
	if (!fListeners.IsEmpty()) {
		// get the table node and the column index
		TreeTableRow* row = dynamic_cast<TreeTableRow*>(_row);
		int32 columnIndex = column->LogicalFieldNum();
		if (row == NULL || columnIndex < 0)
			return;

		// get the node path
		TreeTablePath path;
		_GetPathForNode(row->Node(), path);

		// notify listeners
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--) {
			fListeners.ItemAt(i)->TreeTableCellMouseDown(this, path,
				columnIndex, screenWhere, buttons);
		}
	}
}


void
TreeTable::ColumnMouseUp(AbstractColumn* column, BRow* _row, BField* field,
	BPoint screenWhere, uint32 buttons)
{
	if (!fListeners.IsEmpty()) {
		// get the table node and the column index
		TreeTableRow* row = dynamic_cast<TreeTableRow*>(_row);
		int32 columnIndex = column->LogicalFieldNum();
		if (row == NULL || columnIndex < 0)
			return;

		// get the node path
		TreeTablePath path;
		_GetPathForNode(row->Node(), path);

		// notify listeners
		int32 listenerCount = fListeners.CountItems();
		for (int32 i = listenerCount - 1; i >= 0; i--) {
			fListeners.ItemAt(i)->TreeTableCellMouseUp(this, path, columnIndex,
				screenWhere, buttons);
		}
	}
}


void
TreeTable::TableNodesAdded(TreeTableModel* model, const TreeTablePath& path,
	int32 childIndex, int32 count)
{
	TreeTableNode* node = _NodeForPath(path);
	if (node == NULL)
		return;

	_AddChildRows(node, childIndex, count, fModel->CountColumns());
}


void
TreeTable::TableNodesRemoved(TreeTableModel* model, const TreeTablePath& path,
	int32 childIndex, int32 count)
{
	TreeTableNode* node = _NodeForPath(path);
	if (node == NULL)
		return;

	_RemoveChildRows(node, childIndex, count);
}


void
TreeTable::TableNodesChanged(TreeTableModel* model, const TreeTablePath& path,
	int32 childIndex, int32 count)
{
	TreeTableNode* node = _NodeForPath(path);
	if (node == NULL)
		return;

	int32 endIndex = childIndex + count;
	for (int32 i = childIndex; i < endIndex; i++) {
		if (TreeTableNode* child = node->ChildAt(i))
			UpdateRow(child->Row());
	}
}


void
TreeTable::ExpandOrCollapse(BRow* _row, bool expand)
{
	TreeTableRow* row = dynamic_cast<TreeTableRow*>(_row);
	if (row == NULL || row->IsExpanded() == expand)
		return;

	AbstractTable::ExpandOrCollapse(row, expand);

	if (row->IsExpanded() != expand)
		return;

	TreeTablePath path;
	_GetPathForNode(row->Node(), path);

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TreeTableNodeExpandedChanged(this, path, expand);
}


void
TreeTable::ItemInvoked()
{
	if (fListeners.IsEmpty())
		return;

	TreeTableRow* row = dynamic_cast<TreeTableRow*>(CurrentSelection());
	if (row == NULL)
		return;

	TreeTablePath path;
	_GetPathForNode(row->Node(), path);

	int32 listenerCount = fListeners.CountItems();
	for (int32 i = listenerCount - 1; i >= 0; i--)
		fListeners.ItemAt(i)->TreeTableNodeInvoked(this, path);
}


bool
TreeTable::_AddChildRows(TreeTableNode* parentNode, int32 childIndex,
	int32 count, int32 columnCount)
{
	int32 childEndIndex = childIndex + count;
	for (int32 i = childIndex; i < childEndIndex; i++) {
		void* child = fModel->ChildAt(parentNode->ModelObject(), i);

		// create node
		TreeTableNode* node = new(std::nothrow) TreeTableNode(parentNode);
		if (node == NULL || node->Init(child, columnCount) != B_OK
			|| !parentNode->AddChild(node, i)) {
			delete node;
			return false;
		}

		// add row
		AddRow(node->Row(), i,
			parentNode != fRootNode ? parentNode->Row() : NULL);

		// recursively create children
		if (!_AddChildRows(node, 0, fModel->CountChildren(child), columnCount))
			return false;
	}

	return true;
}


void
TreeTable::_RemoveChildRows(TreeTableNode* parentNode, int32 childIndex,
	int32 count)
{
	for (int32 i = childIndex + count - 1; i >= childIndex; i--) {
		if (TreeTableNode* child = parentNode->RemoveChild(i)) {
			int32 childCount = child->CountChildren();
			if (childCount > 0)
				_RemoveChildRows(child, 0, childCount);

			RemoveRow(child->Row());
			delete child;
		}
	}
}


void
TreeTable::_SetNodeExpanded(TreeTableNode* node, bool expanded,
	bool expandAncestors)
{
	if (expanded && expandAncestors && node != fRootNode)
		_SetNodeExpanded(node->Parent(), true, true);

	ExpandOrCollapse(node->Row(), expanded);
}


TreeTableNode*
TreeTable::_NodeForPath(const TreeTablePath& path) const
{
	TreeTableNode* node = fRootNode;

	int32 count = path.CountComponents();
	for (int32 i = 0; node != NULL && i < count; i++)
		node = node->ChildAt(path.ComponentAt(i));

	return node;
}


void
TreeTable::_GetPathForNode(TreeTableNode* node, TreeTablePath& _path) const
{
	if (node == fRootNode) {
		_path.Clear();
		return;
	}

	_GetPathForNode(node->Parent(), _path);
	_path.AddComponent(node->Parent()->IndexOf(node));
}
