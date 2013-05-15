/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "ImageFunctionsView.h"

#include <stdio.h>

#include <new>
#include <set>

#include <AutoDeleter.h>

#include "table/TableColumns.h"

#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "LocatableFile.h"
#include "Tracing.h"


// #pragma mark - SourcePathComponentNode


class ImageFunctionsView::SourcePathComponentNode : public BReferenceable {
public:
	SourcePathComponentNode(SourcePathComponentNode* parent,
		const BString& componentName, LocatableFile* sourceFile,
		FunctionInstance* function)
		:
		fParent(parent),
		fComponentName(componentName),
		fSourceFile(sourceFile),
		fFunction(function)
	{
		if (fSourceFile != NULL)
			fSourceFile->AcquireReference();
		if (fFunction != NULL)
			fFunction->AcquireReference();
	}

	virtual ~SourcePathComponentNode()
	{
		for (int32 i = 0; i < fChildPathComponents.CountItems(); i++)
			fChildPathComponents.ItemAt(i)->ReleaseReference();

		if (fSourceFile != NULL)
			fSourceFile->ReleaseReference();

		if (fFunction != NULL)
			fFunction->ReleaseReference();
	}

	const BString& ComponentName() const
	{
		return fComponentName;
	}

	LocatableFile* SourceFile() const
	{
		return fSourceFile;
	}

	FunctionInstance* Function() const
	{
		return fFunction;
	}

	int32 CountChildren() const
	{
		return fChildPathComponents.CountItems();
	}

	SourcePathComponentNode* ChildAt(int32 index)
	{
		return fChildPathComponents.ItemAt(index);
	}

	SourcePathComponentNode* FindChildByName(const BString& name) const
	{
		return fChildPathComponents.BinarySearchByKey(name,
			&CompareByComponentName);
	}

	int32 FindChildIndexByName(const BString& name) const
	{
		return fChildPathComponents.BinarySearchIndexByKey(name,
			&CompareByComponentName);
	}

	bool AddChild(SourcePathComponentNode* child)
	{
		if (!fChildPathComponents.BinaryInsert(child,
				&CompareComponents)) {
			return false;
		}

		child->AcquireReference();

		return true;
	}

	bool RemoveChild(SourcePathComponentNode* child)
	{
		if (!fChildPathComponents.RemoveItem(child))
			return false;

		child->ReleaseReference();

		return true;
	}

	bool RemoveAllChildren()
	{
		for (int32 i = 0; i < fChildPathComponents.CountItems(); i++)
			RemoveChild(fChildPathComponents.ItemAt(i));

		return true;
	}

private:
	friend class ImageFunctionsView::FunctionsTableModel;

	static int CompareByComponentName(const BString* name, const
		SourcePathComponentNode* node)
	{
		return name->Compare(node->ComponentName());
	}

	static int CompareComponents(const SourcePathComponentNode* a,
		const SourcePathComponentNode* b)
	{
		return a->ComponentName().Compare(b->ComponentName());
	}

private:
	typedef BObjectList<SourcePathComponentNode> ChildPathComponentList;

private:
	SourcePathComponentNode* fParent;
	BString					fComponentName;
	LocatableFile*			fSourceFile;
	FunctionInstance*		fFunction;
	ChildPathComponentList	fChildPathComponents;
};


// #pragma mark - FunctionsTableModel


class ImageFunctionsView::FunctionsTableModel : public TreeTableModel {
public:
	FunctionsTableModel()
		:
		fImageDebugInfo(NULL),
		fSourcelessNode(NULL)
	{
	}

	~FunctionsTableModel()
	{
		SetImageDebugInfo(NULL);
	}

	void SetImageDebugInfo(ImageDebugInfo* imageDebugInfo)
	{
		// unset old functions
		if (fImageDebugInfo != NULL) {
			for (int32 i = 0; i < fChildPathComponents.CountItems(); i++)
				fChildPathComponents.ItemAt(i)->ReleaseReference();

			fChildPathComponents.MakeEmpty();
			fSourcelessNode = NULL;
		}

		fImageDebugInfo = imageDebugInfo;

		// set new functions
		if (fImageDebugInfo == NULL || fImageDebugInfo->CountFunctions()
				== 0) {
			NotifyNodesCleared();
			return;
		}

		std::set<target_addr_t> functionAddresses;

		SourcePathComponentNode* sourcelessNode = new(std::nothrow)
			SourcePathComponentNode(NULL, "<no source file>", NULL, NULL);
		BReference<SourcePathComponentNode> sourceNodeRef(
			sourcelessNode, true);

		int32 functionCount = fImageDebugInfo->CountFunctions();
		for (int32 i = 0; i < functionCount; i++) {
			FunctionInstance* instance = fImageDebugInfo->FunctionAt(i);
			target_addr_t address = instance->Address();
			if (functionAddresses.find(address) != functionAddresses.end())
				continue;
			else {
				try {
					functionAddresses.insert(address);
				} catch (...) {
					return;
				}
			}

			if (!_BuildFunctionSourcePath(instance, sourcelessNode))
				return;
		}

		if (sourcelessNode->CountChildren() != 0) {
			if (fChildPathComponents.BinaryInsert(sourcelessNode,
					&SourcePathComponentNode::CompareComponents)) {
				fSourcelessNode = sourcelessNode;
				sourceNodeRef.Detach();
			}
		}

		NotifyTableModelReset();
	}

	virtual int32 CountColumns() const
	{
		return 1;
	}

	virtual void* Root() const
	{
		return (void*)this;
	}

	virtual int32 CountChildren(void* parent) const
	{
		if (parent == this)
			return fChildPathComponents.CountItems();

		return ((SourcePathComponentNode*)parent)->CountChildren();
	}

	virtual void* ChildAt(void* parent, int32 index) const
	{
		if (parent == this)
			return fChildPathComponents.ItemAt(index);

		return ((SourcePathComponentNode*)parent)->ChildAt(index);
	}

	virtual bool GetValueAt(void* object, int32 columnIndex, BVariant& value)
	{
		if (columnIndex != 0)
			return false;

		if (object == this)
			return false;

		SourcePathComponentNode* node = (SourcePathComponentNode*)object;

		value.SetTo(node->ComponentName(), B_VARIANT_DONT_COPY_DATA);

		return true;
	}

	bool GetFunctionPath(FunctionInstance* function, TreeTablePath& _path)
	{
		if (function == NULL)
			return false;

		LocatableFile* sourceFile = function->SourceFile();
		SourcePathComponentNode* node = NULL;
		int32 childIndex = -1;
		if (sourceFile == NULL) {
			node = fSourcelessNode;
			_path.AddComponent(fChildPathComponents.IndexOf(node));
		} else {
			BString sourcePath;
			sourceFile->GetPath(sourcePath);

			if (sourcePath.IsEmpty())
				return false;

			BString searchPath;
			while (!sourcePath.IsEmpty()) {
				if (sourcePath[0] == '/')
					sourcePath.Remove(0, 1);
				int32 separatorIndex = sourcePath.FindFirst('/');
				if (separatorIndex == -1) {
					searchPath = sourcePath;
					sourcePath.Truncate(0);
				} else
					sourcePath.MoveInto(searchPath, 0, separatorIndex);

				if (node == NULL) {
					childIndex = fChildPathComponents.BinarySearchIndexByKey(
						searchPath,
						&SourcePathComponentNode::CompareByComponentName);
					node = fChildPathComponents.ItemAt(childIndex);
				} else {
					childIndex = node->FindChildIndexByName(searchPath);
					node = node->ChildAt(childIndex);
				}

				if (childIndex < 0)
					return false;

				_path.AddComponent(childIndex);
			}
		}

		if (node == NULL)
			return false;

		childIndex = node->FindChildIndexByName(function->PrettyName());
		if (childIndex < 0)
			return false;

		_path.AddComponent(childIndex);
		return true;
	}

	bool GetObjectForPath(const TreeTablePath& path,
		LocatableFile*& _sourceFile, FunctionInstance*& _function)
	{
		SourcePathComponentNode* node = fChildPathComponents.ItemAt(
			path.ComponentAt(0));

		if (node == NULL)
			return false;

		for (int32 i = 1; i < path.CountComponents(); i++)
			node = node->ChildAt(path.ComponentAt(i));

		if (node != NULL) {
			_sourceFile = node->SourceFile();
			_function = node->Function();
			return true;
		}

		return false;
	}

private:
	bool _BuildFunctionSourcePath(FunctionInstance* function,
		SourcePathComponentNode* sourcelessNode)
	{
		LocatableFile* sourceFile = function->SourceFile();
		if (sourceFile == NULL)
			return _AddFunctionNode(sourcelessNode, function, NULL);

		BString sourcePath;
		sourceFile->GetPath(sourcePath);
		if (sourcePath.IsEmpty())
			return false;

		return _AddNextPathComponent(NULL, sourcePath, function, sourceFile);
	}

	bool _AddNextPathComponent(SourcePathComponentNode* parent,
		BString& childPath, FunctionInstance* function, LocatableFile* file)
	{
		if (childPath[0] == '/')
			childPath.Remove(0, 1);

		BString pathComponent;
		int32 pathSeparatorIndex = childPath.FindFirst('/');
		if (pathSeparatorIndex == -1)
			pathComponent = childPath;
		else
			childPath.MoveInto(pathComponent, 0, pathSeparatorIndex);

		SourcePathComponentNode* currentNode = NULL;
		if (parent == NULL) {
			currentNode = fChildPathComponents.BinarySearchByKey(pathComponent,
				SourcePathComponentNode::CompareByComponentName);
		} else
			currentNode = parent->FindChildByName(pathComponent);

		if (currentNode == NULL) {
			currentNode = new(std::nothrow) SourcePathComponentNode(parent,
				pathComponent, NULL, NULL);
			if (currentNode == NULL)
				return false;
			BReference<SourcePathComponentNode> nodeReference(currentNode,
				true);
			if (parent != NULL) {
				if (!parent->AddChild(currentNode))
					return false;
			} else {
				if (!fChildPathComponents.BinaryInsert(currentNode,
					&SourcePathComponentNode::CompareComponents)) {
					return false;
				}

				nodeReference.Detach();
			}
		}

		if (pathSeparatorIndex == -1)
			return _AddFunctionNode(currentNode, function, file);

		return _AddNextPathComponent(currentNode, childPath, function, file);
	}

	bool _AddFunctionNode(SourcePathComponentNode* parent,
		FunctionInstance* function, LocatableFile* file)
	{
		SourcePathComponentNode* functionNode = new(std::nothrow)
			SourcePathComponentNode(parent, function->PrettyName(), file,
				function);

		if (functionNode == NULL)
			return B_NO_MEMORY;

		BReference<SourcePathComponentNode> nodeReference(functionNode, true);
		if (!parent->AddChild(functionNode))
			return false;

		return true;
	}

private:
	typedef BObjectList<SourcePathComponentNode> ChildPathComponentList;

private:
	ImageDebugInfo*			fImageDebugInfo;
	ChildPathComponentList	fChildPathComponents;
	SourcePathComponentNode* fSourcelessNode;
};


// #pragma mark - ImageFunctionsView


ImageFunctionsView::ImageFunctionsView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fImageDebugInfo(NULL),
	fFunctionsTable(NULL),
	fFunctionsTableModel(NULL),
	fListener(listener)
{
	SetName("Functions");
}


ImageFunctionsView::~ImageFunctionsView()
{
	SetImageDebugInfo(NULL);
	fFunctionsTable->SetTreeTableModel(NULL);
	delete fFunctionsTableModel;
}


/*static*/ ImageFunctionsView*
ImageFunctionsView::Create(Listener* listener)
{
	ImageFunctionsView* self = new ImageFunctionsView(listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
ImageFunctionsView::UnsetListener()
{
	fListener = NULL;
}


void
ImageFunctionsView::SetImageDebugInfo(ImageDebugInfo* imageDebugInfo)
{
	if (imageDebugInfo == fImageDebugInfo)
		return;

	TRACE_GUI("ImageFunctionsView::SetImageDebugInfo(%p)\n", imageDebugInfo);

	if (fImageDebugInfo != NULL)
		fImageDebugInfo->ReleaseReference();

	fImageDebugInfo = imageDebugInfo;

	if (fImageDebugInfo != NULL)
		fImageDebugInfo->AcquireReference();

	fFunctionsTableModel->SetImageDebugInfo(fImageDebugInfo);

	// If there's only one source file (i.e. "no source file"), expand the item.
	if (fImageDebugInfo != NULL
		&& fFunctionsTableModel->CountChildren(fFunctionsTableModel) == 1) {
		TreeTablePath path;
		path.AddComponent(0);
		fFunctionsTable->SetNodeExpanded(path, true, false);
	}

	if (fImageDebugInfo != NULL)
		fFunctionsTable->ResizeAllColumnsToPreferred();

	TRACE_GUI("ImageFunctionsView::SetImageDebugInfo(%p) done\n",
		imageDebugInfo);
}


void
ImageFunctionsView::SetFunction(FunctionInstance* function)
{
	TRACE_GUI("ImageFunctionsView::SetFunction(%p)\n", function);

	TreeTablePath path;
	if (fFunctionsTableModel->GetFunctionPath(function, path)) {
		fFunctionsTable->SetNodeExpanded(path, true, true);
		fFunctionsTable->SelectNode(path, false);
		fFunctionsTable->ScrollToNode(path);
		fFunctionsTable->ResizeAllColumnsToPreferred();
	} else
		fFunctionsTable->DeselectAllNodes();
}


void
ImageFunctionsView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("functionsTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fFunctionsTable);
	}
}


status_t
ImageFunctionsView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
		fFunctionsTable);
	if (result == B_OK)
		result = settings.AddMessage("functionsTable", &tableSettings);

	return result;
}


void
ImageFunctionsView::TreeTableSelectionChanged(TreeTable* table)
{
	if (fListener == NULL)
		return;

	LocatableFile* sourceFile = NULL;
	FunctionInstance* function = NULL;
	TreeTablePath path;
	if (table->SelectionModel()->GetPathAt(0, path))
		fFunctionsTableModel->GetObjectForPath(path, sourceFile, function);

	fListener->FunctionSelectionChanged(function);
}


void
ImageFunctionsView::_Init()
{
	fFunctionsTable = new TreeTable("functions", 0, B_FANCY_BORDER);
	AddChild(fFunctionsTable->ToView());
	fFunctionsTable->SetSortingEnabled(false);

	// columns
	fFunctionsTable->AddColumn(new StringTableColumn(0, "File/Function", 300,
		100, 1000, B_TRUNCATE_BEGINNING, B_ALIGN_LEFT));

	fFunctionsTableModel = new FunctionsTableModel();
	fFunctionsTable->SetTreeTableModel(fFunctionsTableModel);

	fFunctionsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fFunctionsTable->AddTreeTableListener(this);
}


// #pragma mark - Listener


ImageFunctionsView::Listener::~Listener()
{
}
