/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2012, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TREE_TABLE_H
#define TREE_TABLE_H

#include <vector>

#include <ColumnTypes.h>
#include <Variant.h>

#include "table/AbstractTable.h"
#include "table/TableColumn.h"


class TreeTable;
class TreeTableModel;
class TreeTableNode;
class TreeTableRow;


class TreeTablePath {
public:
								TreeTablePath();
								TreeTablePath(const TreeTablePath& other);
								TreeTablePath(const TreeTablePath& other,
									int32 childIndex);
								~TreeTablePath();

		bool					AddComponent(int32 childIndex);
		int32					RemoveLastComponent();
		void					Clear();

		int32					CountComponents() const;
		int32					ComponentAt(int32 index) const;

		TreeTablePath&			operator=(const TreeTablePath& other);
		bool					operator==(const TreeTablePath& other) const;
		bool					operator!=(const TreeTablePath& other) const;

private:
		typedef std::vector<int32> ComponentVector;

private:
		ComponentVector			fComponents;
};


class TreeTableModelListener {
public:
	virtual						~TreeTableModelListener();

	virtual	void				TableNodesAdded(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);
	virtual	void				TableNodesRemoved(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);
	virtual	void				TableNodesChanged(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);
};


class TreeTableModel : public AbstractTableModelBase {
public:
	virtual						~TreeTableModel();

	virtual	void*				Root() const = 0;
									// the root itself isn't shown

	virtual	int32				CountChildren(void* parent) const = 0;
	virtual	void*				ChildAt(void* parent, int32 index) const = 0;

	virtual	bool				GetValueAt(void* object, int32 columnIndex,
									BVariant& value) = 0;

	virtual	void*				NodeForPath(const TreeTablePath& path) const;

	virtual	bool				AddListener(TreeTableModelListener* listener);
	virtual	void				RemoveListener(
									TreeTableModelListener* listener);

protected:
			typedef BObjectList<TreeTableModelListener> ListenerList;

protected:
			void				NotifyNodesAdded(const TreeTablePath& path,
									int32 childIndex, int32 count);
			void				NotifyNodesRemoved(const TreeTablePath& path,
									int32 childIndex, int32 count);
			void				NotifyNodesChanged(const TreeTablePath& path,
									int32 childIndex, int32 count);

protected:
			ListenerList		fListeners;
};


class TreeTableSelectionModel {
public:
								TreeTableSelectionModel(TreeTable* table);
								~TreeTableSelectionModel();

			int32				CountNodes();
			void*				NodeAt(int32 index);
			bool				GetPathAt(int32 index, TreeTablePath& _path);

private:
			friend class TreeTable;

private:
			void				_SelectionChanged();
			void				_Update();
			TreeTableNode*		_NodeAt(int32 index);

private:
			TreeTable*			fTreeTable;
			TreeTableNode**		fNodes;
			int32				fNodeCount;
};


class TreeTableToolTipProvider {
public:
	virtual						~TreeTableToolTipProvider();

	virtual	bool				GetToolTipForTablePath(
									const TreeTablePath& path,
									int32 columnIndex, BToolTip** _tip) = 0;
									// columnIndex can be -1, if not in a column
};


class TreeTableListener {
public:
	virtual						~TreeTableListener();

	virtual	void				TreeTableSelectionChanged(TreeTable* table);
	virtual	void				TreeTableNodeInvoked(TreeTable* table,
									const TreeTablePath& path);
	virtual	void				TreeTableNodeExpandedChanged(TreeTable* table,
									const TreeTablePath& path, bool expanded);

	virtual	void				TreeTableCellMouseDown(TreeTable* table,
									const TreeTablePath& path,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);
	virtual	void				TreeTableCellMouseUp(TreeTable* table,
									const TreeTablePath& path,
									int32 columnIndex, BPoint screenWhere,
									uint32 buttons);
};


class TreeTable : public AbstractTable, private TreeTableModelListener {
public:
								TreeTable(const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
								TreeTable(TreeTableModel* model,
									const char* name, uint32 flags,
									border_style borderStyle = B_NO_BORDER,
									bool showHorizontalScrollbar = true);
	virtual						~TreeTable();

			bool				SetTreeTableModel(TreeTableModel* model);
			TreeTableModel*		GetTreeTableModel() const	{ return fModel; }

			void				SetToolTipProvider(
									TreeTableToolTipProvider* toolTipProvider);
			TreeTableToolTipProvider* ToolTipProvider() const
									{ return fToolTipProvider; }

			TreeTableSelectionModel* SelectionModel();

			void				SelectNode(const TreeTablePath& path,
									bool extendSelection);
			void				DeselectNode(const TreeTablePath& path);
			void				DeselectAllNodes();

			bool				IsNodeExpanded(const TreeTablePath& path) const;
			void				SetNodeExpanded(const TreeTablePath& path,
									bool expanded,
									bool expandAncestors = false);

			void				ScrollToNode(const TreeTablePath& path);

			bool				AddTreeTableListener(
									TreeTableListener* listener);
			void				RemoveTreeTableListener(
									TreeTableListener* listener);

protected:
	virtual bool				GetToolTipAt(BPoint point, BToolTip** _tip);

	virtual	void				SelectionChanged();

	virtual	AbstractColumn*		CreateColumn(TableColumn* column);

	virtual	void				ColumnMouseDown(AbstractColumn* column,
									BRow* row, BField* field,
									BPoint screenWhere, uint32 buttons);
	virtual	void				ColumnMouseUp(AbstractColumn* column,
									BRow* row, BField* field,
									BPoint screenWhere, uint32 buttons);

private:
	virtual	void				TableNodesAdded(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);
	virtual	void				TableNodesRemoved(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);
	virtual	void				TableNodesChanged(TreeTableModel* model,
									const TreeTablePath& path, int32 childIndex,
									int32 count);

private:
			class Column;

			friend class TreeTableSelectionModel;

			typedef BObjectList<TreeTableListener>	ListenerList;

private:
	virtual	void				ExpandOrCollapse(BRow* row, bool expand);
	virtual	void				ItemInvoked();

			bool				_AddChildRows(TreeTableNode* parentNode,
									int32 childIndex, int32 count,
									int32 columnCount);
			void				_RemoveChildRows(TreeTableNode* parentNode,
									int32 childIndex, int32 count);

			void				_SetNodeExpanded(TreeTableNode* node,
									bool expanded,
									bool expandAncestors = false);

			TreeTableNode*		_NodeForPath(const TreeTablePath& path) const;
			void				_GetPathForNode(TreeTableNode* node,
									TreeTablePath& _path) const;

private:
			TreeTableModel*		fModel;
			TreeTableToolTipProvider* fToolTipProvider;
			TreeTableNode*		fRootNode;
			TreeTableSelectionModel fSelectionModel;
			ListenerList		fListeners;
			int32				fIgnoreSelectionChange;
};


#endif	// TREE_TABLE_H
