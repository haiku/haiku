/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ImageFunctionsView.h"

#include <stdio.h>

#include <algorithm>
#include <new>

#include <AutoDeleter.h>

#include "table/TableColumns.h"

#include "FunctionDebugInfo.h"
#include "Image.h"
#include "ImageDebugInfo.h"


// #pragma mark - FunctionsTableModel


class ImageFunctionsView::FunctionsTableModel : public TreeTableModel {
public:
	FunctionsTableModel()
		:
		fImageDebugInfo(NULL),
		fFunctions(NULL),
		fFunctionCount(0),
		fSourceFileIndices(NULL),
		fSourceFileCount(0)
	{
	}

	~FunctionsTableModel()
	{
		SetImageDebugInfo(NULL);
	}

	void SetImageDebugInfo(ImageDebugInfo* imageDebugInfo)
	{
		// unset old functions
		if (fSourceFileIndices != NULL) {
			NotifyNodesRemoved(TreeTablePath(), 0, fSourceFileCount);

			delete[] fFunctions;
			fFunctions = NULL;
			fFunctionCount = 0;

			delete[] fSourceFileIndices;
			fSourceFileIndices = NULL;
			fSourceFileCount = 0;
		}

		fImageDebugInfo = imageDebugInfo;

		// set new functions
		if (fImageDebugInfo == NULL || fImageDebugInfo->CountFunctions() == 0)
			return;

		// create an array with the functions
		int32 functionCount = fImageDebugInfo->CountFunctions();
		FunctionDebugInfo** functions
			= new(std::nothrow) FunctionDebugInfo*[functionCount];
		if (functions == NULL)
			return;
		ArrayDeleter<FunctionDebugInfo*> functionsDeleter(functions);

		for (int32 i = 0; i < functionCount; i++)
			functions[i] = fImageDebugInfo->FunctionAt(i);

		// sort them
		std::sort(functions, functions + functionCount, &_FunctionLess);

		// count the different source files
		int32 sourceFileCount = 1;
		for (int32 i = 1; i < functionCount; i++) {
			if (_CompareSourceFileNames(functions[i - 1]->SourceFileName(),
					functions[i]->SourceFileName()) != 0) {
				sourceFileCount++;
			}
		}

		// allocate and init the indices for the source files
		fSourceFileIndices = new(std::nothrow) int32[sourceFileCount];
		if (fSourceFileIndices == NULL)
			return;
		fSourceFileCount = sourceFileCount;

		fSourceFileIndices[0] = 0;

		int32 sourceFileIndex = 1;
		for (int32 i = 1; i < functionCount; i++) {
			if (_CompareSourceFileNames(functions[i - 1]->SourceFileName(),
					functions[i]->SourceFileName()) != 0) {
				fSourceFileIndices[sourceFileIndex++] = i;
			}
		}

		fFunctions = functionsDeleter.Detach();
		fFunctionCount = functionCount;

		NotifyNodesAdded(TreeTablePath(), 0, fSourceFileCount);
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
			return fSourceFileCount;

		if (parent >= fSourceFileIndices
			&& parent < fSourceFileIndices + fSourceFileCount) {
			int32 sourceIndex = (int32*)parent - fSourceFileIndices;
			return _CountSourceFileFunctions(sourceIndex);
		}

		return 0;
	}

	virtual void* ChildAt(void* parent, int32 index) const
	{
		if (parent == this) {
			return index >= 0 && index < fSourceFileCount
				? fSourceFileIndices + index : NULL;
		}

		if (parent >= fSourceFileIndices
			&& parent < fSourceFileIndices + fSourceFileCount) {
			int32 sourceIndex = (int32*)parent - fSourceFileIndices;
			int32 count = _CountSourceFileFunctions(sourceIndex);
			int32 firstFunctionIndex = fSourceFileIndices[sourceIndex];
			return index >= 0 && index < count
				? fFunctions[firstFunctionIndex + index] : NULL;
		}

		return NULL;
	}

	virtual bool GetValueAt(void* object, int32 columnIndex, BVariant& value)
	{
		if (columnIndex != 0)
			return false;

		if (object == this)
			return false;

		if (object >= fSourceFileIndices
			&& object < fSourceFileIndices + fSourceFileCount) {
			const char* name = fFunctions[*(int32*)object]->SourceFileName();
			value.SetTo(name != NULL ? name : "<no source file>",
				B_VARIANT_DONT_COPY_DATA);
			return true;
		}

		FunctionDebugInfo* function = (FunctionDebugInfo*)object;
		value.SetTo(function->PrettyName(), B_VARIANT_DONT_COPY_DATA);
		return true;
	}

	bool GetFunctionPath(FunctionDebugInfo* function, TreeTablePath& _path)
	{
		int32 index = -1;
		for (int32 i = 0; i < fFunctionCount; i++) {
			if (fFunctions[i] == function) {
				index = i;
				break;
			}
		}

		if (index < 0)
			return false;

		int32 sourceIndex = fSourceFileCount - 1;
		while (fSourceFileIndices[sourceIndex] > index)
			sourceIndex--;

		_path.Clear();
		return _path.AddComponent(sourceIndex)
			&& _path.AddComponent(index - fSourceFileIndices[sourceIndex]);
	}

	bool GetObjectForPath(const TreeTablePath& path, const char*& _sourceFile,
		FunctionDebugInfo*& _function)
	{
		int32 componentCount = path.CountComponents();
		if (componentCount == 0 || componentCount > 2)
			return false;

		int32 sourceIndex = path.ComponentAt(0);
		if (sourceIndex < 0 || sourceIndex >= fSourceFileCount)
			return false;

		_sourceFile = fFunctions[fSourceFileIndices[sourceIndex]]
			->SourceFileName();

		_function = NULL;

		if (componentCount == 2) {
			int32 index = path.ComponentAt(1);
			if (index >= 0 && index < _CountSourceFileFunctions(sourceIndex))
				_function = fFunctions[fSourceFileIndices[sourceIndex] + index];
		}

		return true;
	}

	int32 CountSourceFiles() const
	{
		return fSourceFileCount;
	}

private:
	int32 _CountSourceFileFunctions(int32 sourceIndex) const
	{
		if (sourceIndex < 0 || sourceIndex >= fSourceFileCount)
			return 0;

		int32 nextFunctionIndex = sourceIndex + 1 < fSourceFileCount
			? fSourceFileIndices[sourceIndex + 1] : fFunctionCount;
		return nextFunctionIndex - fSourceFileIndices[sourceIndex];
	}

	static int _CompareSourceFileNames(const char* a, const char* b)
	{
		if (a == b)
			return 0;

		if (a == NULL)
			return 1;
		if (b == NULL)
			return -1;

		return strcmp(a, b);
	}

	static bool _FunctionLess(const FunctionDebugInfo* a,
		const FunctionDebugInfo* b)
	{
		// compare source file name first
		int compared = _CompareSourceFileNames(a->SourceFileName(),
			b->SourceFileName());
		if (compared != 0)
			return compared < 0;

		// source file names are equal -- compare the function names
		return strcasecmp(a->PrettyName(), b->PrettyName()) < 0;
	}

private:
	ImageDebugInfo*		fImageDebugInfo;
	FunctionDebugInfo**	fFunctions;
	int32				fFunctionCount;
	int32*				fSourceFileIndices;
	int32				fSourceFileCount;
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
printf("ImageFunctionsView::SetImageDebugInfo(%p)\n", imageDebugInfo);

	if (fImageDebugInfo != NULL)
		fImageDebugInfo->RemoveReference();

	fImageDebugInfo = imageDebugInfo;

	if (fImageDebugInfo != NULL)
		fImageDebugInfo->AddReference();

	fFunctionsTableModel->SetImageDebugInfo(fImageDebugInfo);

	// If there's only one source file (i.e. "no source file"), expand the item.
	if (fImageDebugInfo != NULL
		&& fFunctionsTableModel->CountSourceFiles() == 1) {
		TreeTablePath path;
		path.AddComponent(0);
		fFunctionsTable->SetNodeExpanded(path, true, false);
	}

printf("ImageFunctionsView::SetImageDebugInfo(%p) done\n", imageDebugInfo);
}


void
ImageFunctionsView::SetFunction(FunctionDebugInfo* function)
{
printf("ImageFunctionsView::SetFunction(%p)\n", function);
	TreeTablePath path;
	if (fFunctionsTableModel->GetFunctionPath(function, path)) {
		fFunctionsTable->SetNodeExpanded(path, true, true);
		fFunctionsTable->SelectNode(path, false);
		fFunctionsTable->ScrollToNode(path);
	} else
		fFunctionsTable->DeselectAllNodes();
}


void
ImageFunctionsView::TreeTableSelectionChanged(TreeTable* table)
{
	if (fListener == NULL)
		return;

	const char* sourceFile = NULL;
	FunctionDebugInfo* function = NULL;
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
		100, 1000, B_TRUNCATE_END, B_ALIGN_LEFT));

	fFunctionsTableModel = new FunctionsTableModel();
	fFunctionsTable->SetTreeTableModel(fFunctionsTableModel);

	fFunctionsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fFunctionsTable->AddTreeTableListener(this);
}


// #pragma mark - Listener


ImageFunctionsView::Listener::~Listener()
{
}
