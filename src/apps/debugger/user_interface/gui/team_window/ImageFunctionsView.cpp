/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2015, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "ImageFunctionsView.h"

#include <stdio.h>

#include <new>
#include <set>

#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <StringList.h>
#include <TextControl.h>

#include <AutoDeleter.h>
#include <RegExp.h>

#include "table/TableColumns.h"

#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "Image.h"
#include "ImageDebugInfo.h"
#include "LocatableFile.h"
#include "TargetAddressTableColumn.h"
#include "Tracing.h"


static const uint32 MSG_FUNCTION_FILTER_CHANGED = 'mffc';
static const uint32 MSG_FUNCTION_TYPING_TIMEOUT	= 'mftt';

static const uint32 kKeypressTimeout = 250000;

// from ColumnTypes.cpp
static const float kTextMargin = 8.0;


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
		fFunction(function),
		fFilterMatch(),
		fHasMatchingChild(false)
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

	const RegExp::MatchResult& FilterMatch() const
	{
		return fFilterMatch;
	}

	void SetFilterMatch(const RegExp::MatchResult& match)
	{
		fFilterMatch = match;
	}

	bool HasMatchingChild() const
	{
		return fHasMatchingChild;
	}

	void SetHasMatchingChild()
	{
		fHasMatchingChild = true;
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
	RegExp::MatchResult		fFilterMatch;
	bool					fHasMatchingChild;
};


// #pragma mark - HighlightingTableColumn


class ImageFunctionsView::HighlightingTableColumn : public StringTableColumn {
public:
	HighlightingTableColumn(int32 modelIndex, const char* title, float width,
		float minWidth, float maxWidth, uint32 truncate,
		alignment align = B_ALIGN_LEFT)
		:
		StringTableColumn(modelIndex, title, width, minWidth, maxWidth,
			truncate, align),
		fHasFilter(false)
	{
	}

	void SetHasFilter(bool hasFilter)
	{
		fHasFilter = hasFilter;
	}

	virtual void DrawValue(const BVariant& value, BRect rect,
		BView* targetView)
	{
		StringTableColumn::DrawValue(value, rect, targetView);

		if (fHasFilter) {
			// TODO: handle this case as well
			if (fField.HasClippedString())
				return;

			const SourcePathComponentNode* node
				= (const SourcePathComponentNode*)value.ToPointer();

			const RegExp::MatchResult& match = node->FilterMatch();
			if (!match.HasMatched())
				return;

			targetView->PushState();
			BRect fillRect(rect);
			fillRect.left += kTextMargin + targetView->StringWidth(
				fField.String(), match.StartOffset());
			float filterWidth = targetView->StringWidth(fField.String()
					+ match.StartOffset(), match.EndOffset()
					- match.StartOffset());
			fillRect.right = fillRect.left + filterWidth;
			targetView->SetLowColor(255, 255, 0, 255);
			targetView->SetDrawingMode(B_OP_MIN);
			targetView->FillRect(fillRect, B_SOLID_LOW);
			targetView->PopState();
		}
	}

	virtual	BField*	PrepareField(const BVariant& value) const
	{
		const SourcePathComponentNode* node
			= (const SourcePathComponentNode*)value.ToPointer();

		BVariant tempValue(node->ComponentName(), B_VARIANT_DONT_COPY_DATA);
		return StringTableColumn::PrepareField(tempValue);
	}


private:
	bool fHasFilter;
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
		int32 count = fChildPathComponents.CountItems();
		if (fImageDebugInfo != NULL) {
			for (int32 i = 0; i < count; i++)
				fChildPathComponents.ItemAt(i)->ReleaseReference();

			fChildPathComponents.MakeEmpty();
			fSourcelessNode = NULL;
		}

		fImageDebugInfo = imageDebugInfo;

		// set new functions
		if (fImageDebugInfo == NULL || fImageDebugInfo->CountFunctions()
				== 0) {
			NotifyNodesRemoved(TreeTablePath(), 0, count);
			return;
		}

		std::set<target_addr_t> functionAddresses;

		SourcePathComponentNode* sourcelessNode = new(std::nothrow)
			SourcePathComponentNode(NULL, "<no source file>", NULL, NULL);
		BReference<SourcePathComponentNode> sourceNodeRef(
			sourcelessNode, true);

		LocatableFile* currentFile = NULL;
		BStringList pathComponents;
		bool applyFilter = !fFilterString.IsEmpty()
			&& fCurrentFilter.IsValid();
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

			LocatableFile* sourceFile = instance->SourceFile();
			BString sourcePath;
			if (sourceFile != NULL)
				sourceFile->GetPath(sourcePath);

			RegExp::MatchResult pathMatch;
			RegExp::MatchResult functionMatch;
			if (applyFilter && !_FilterFunction(instance, sourcePath,
					pathMatch, functionMatch)) {
				continue;
			}

			if (sourceFile == NULL) {
				if (!_AddFunctionNode(sourcelessNode, instance, NULL,
						functionMatch)) {
					return;
				}
				continue;
			}

			if (sourceFile != currentFile) {
				currentFile = sourceFile;
				pathComponents.MakeEmpty();
				if (applyFilter) {
					pathComponents.Add(sourcePath);
				} else {
					if (!_GetSourcePathComponents(currentFile,
						pathComponents)) {
						return;
					}
				}
			}

			if (!_AddFunctionByPath(pathComponents, instance, currentFile,
					pathMatch, functionMatch)) {
				return;
			}
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
		return 2;
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
		if (object == this)
			return false;

		SourcePathComponentNode* node = (SourcePathComponentNode*)object;
		switch (columnIndex) {
			case 0:
			{
				value.SetTo(node);
				break;
			}
			case 1:
			{
				FunctionInstance* function = node->Function();
				if (function == NULL)
					return false;
				value.SetTo(function->Address());
				break;
			}
			default:
				return false;
		}

		return true;
	}

	bool HasMatchingChildAt(void* parent, int32 index) const
	{
		SourcePathComponentNode* node
			= (SourcePathComponentNode*)ChildAt(parent, index);
		if (node != NULL)
			return node->HasMatchingChild();

		return false;
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
			BStringList pathComponents;
			if (!_GetSourcePathComponents(sourceFile, pathComponents))
				return false;

			for (int32 i = 0; i < pathComponents.CountStrings(); i++) {
				BString component = pathComponents.StringAt(i);

				if (node == NULL) {
					childIndex = fChildPathComponents.BinarySearchIndexByKey(
						component,
						&SourcePathComponentNode::CompareByComponentName);
					node = fChildPathComponents.ItemAt(childIndex);
				} else {
					childIndex = node->FindChildIndexByName(component);
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

	void SetFilter(const char* filter)
	{
		fFilterString = filter;
		if (fFilterString.IsEmpty()
			|| fCurrentFilter.SetPattern(filter, RegExp::PATTERN_TYPE_WILDCARD,
				false)) {
			SetImageDebugInfo(fImageDebugInfo);
		}
	}

private:
	bool _GetSourcePathComponents(LocatableFile* currentFile,
		BStringList& pathComponents)
	{
		BString sourcePath;
		currentFile->GetPath(sourcePath);
		if (sourcePath.IsEmpty())
			return false;

		int32 startIndex = 0;
		if (sourcePath[0] == '/')
			startIndex = 1;

		while (startIndex < sourcePath.Length()) {
			int32 searchIndex = sourcePath.FindFirst('/', startIndex);
			BString data;
			if (searchIndex < 0)
				searchIndex = sourcePath.Length();

			sourcePath.CopyInto(data, startIndex, searchIndex - startIndex);
			if (!pathComponents.Add(data))
				return false;

			startIndex = searchIndex + 1;
		}

		return true;
	}

	bool _AddFunctionByPath(const BStringList& pathComponents,
		FunctionInstance* function, LocatableFile* file,
		RegExp::MatchResult& pathMatch, RegExp::MatchResult& functionMatch)
	{
		SourcePathComponentNode* parentNode = NULL;
		SourcePathComponentNode* currentNode = NULL;
		for (int32 i = 0; i < pathComponents.CountStrings(); i++) {
			const BString pathComponent = pathComponents.StringAt(i);
			if (parentNode == NULL) {
				currentNode = fChildPathComponents.BinarySearchByKey(
					pathComponent,
					SourcePathComponentNode::CompareByComponentName);
			} else
				currentNode = parentNode->FindChildByName(pathComponent);

			if (currentNode == NULL) {
				currentNode = new(std::nothrow) SourcePathComponentNode(
					parentNode,	pathComponent, NULL, NULL);
				if (currentNode == NULL)
					return false;

				if (pathComponents.CountStrings() == 1)
					currentNode->SetFilterMatch(pathMatch);

				BReference<SourcePathComponentNode> nodeReference(currentNode,
					true);
				if (parentNode != NULL) {
					if (!parentNode->AddChild(currentNode))
						return false;
				} else {
					if (!fChildPathComponents.BinaryInsert(currentNode,
						&SourcePathComponentNode::CompareComponents)) {
						return false;
					}

					nodeReference.Detach();
				}
			}

			if (functionMatch.HasMatched())
				currentNode->SetHasMatchingChild();

			parentNode = currentNode;

		}

		return _AddFunctionNode(currentNode, function, file,
			functionMatch);
	}

	bool _AddFunctionNode(SourcePathComponentNode* parent,
		FunctionInstance* function, LocatableFile* file,
		RegExp::MatchResult& match)
	{
		SourcePathComponentNode* functionNode = new(std::nothrow)
			SourcePathComponentNode(parent, function->PrettyName(), file,
				function);

		if (functionNode == NULL)
			return B_NO_MEMORY;

		functionNode->SetFilterMatch(match);

		BReference<SourcePathComponentNode> nodeReference(functionNode, true);
		if (!parent->AddChild(functionNode))
			return false;

		return true;
	}

	bool _FilterFunction(FunctionInstance* instance, const BString& sourcePath,
		RegExp::MatchResult& pathMatch, RegExp::MatchResult& functionMatch)
	{
		functionMatch = fCurrentFilter.Match(instance->PrettyName());
		pathMatch = fCurrentFilter.Match(sourcePath.String());

		return functionMatch.HasMatched() || pathMatch.HasMatched();
	}


private:
	typedef BObjectList<SourcePathComponentNode> ChildPathComponentList;

private:
	ImageDebugInfo*			fImageDebugInfo;
	ChildPathComponentList	fChildPathComponents;
	SourcePathComponentNode* fSourcelessNode;
	BString					fFilterString;
	RegExp					fCurrentFilter;
};


// #pragma mark - ImageFunctionsView


ImageFunctionsView::ImageFunctionsView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fImageDebugInfo(NULL),
	fFilterField(NULL),
	fFunctionsTable(NULL),
	fFunctionsTableModel(NULL),
	fListener(listener),
	fHighlightingColumn(NULL),
	fLastFilterKeypress(0)
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
ImageFunctionsView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fFilterField->SetTarget(this);
}


void
ImageFunctionsView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_FUNCTION_FILTER_CHANGED:
		{
			fLastFilterKeypress = system_time();
			BMessage keypressMessage(MSG_FUNCTION_TYPING_TIMEOUT);
			BMessageRunner::StartSending(BMessenger(this), &keypressMessage,
				kKeypressTimeout, 1);
			break;
		}

		case MSG_FUNCTION_TYPING_TIMEOUT:
		{
			if (system_time() - fLastFilterKeypress >= kKeypressTimeout) {
				fFunctionsTableModel->SetFilter(fFilterField->Text());
				fHighlightingColumn->SetHasFilter(
					fFilterField->TextView()->TextLength() > 0);
				_ExpandFilteredNodes();
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
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
	AddChild(fFilterField = new BTextControl("filtertext", "Filter:",
			NULL, NULL));

	fFilterField->SetModificationMessage(new BMessage(
			MSG_FUNCTION_FILTER_CHANGED));
	fFunctionsTable->SetSortingEnabled(false);

	float addressWidth = be_plain_font->StringWidth("0x00000000")
		+ be_control_look->DefaultLabelSpacing() * 2 + 5;

	// columns
	fFunctionsTable->AddColumn(fHighlightingColumn
		= new HighlightingTableColumn(0, "File/Function", 300, 100, 1000,
			B_TRUNCATE_BEGINNING, B_ALIGN_LEFT));
	fFunctionsTable->AddColumn(new TargetAddressTableColumn(1, "Address",
		addressWidth, 40, 1000, B_TRUNCATE_END, B_ALIGN_RIGHT));

	fFunctionsTableModel = new FunctionsTableModel();
	fFunctionsTable->SetTreeTableModel(fFunctionsTableModel);

	fFunctionsTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fFunctionsTable->AddTreeTableListener(this);
}


void
ImageFunctionsView::_ExpandFilteredNodes()
{
	if (fFilterField->TextView()->TextLength() == 0)
		return;

	for (int32 i = 0; i < fFunctionsTableModel->CountChildren(
		fFunctionsTableModel); i++) {
		// only expand nodes if the match actually hit a function,
		// and not just the containing path.
		if (fFunctionsTableModel->CountChildren(fFunctionsTableModel) == 1
			|| fFunctionsTableModel->HasMatchingChildAt(fFunctionsTableModel,
				i)) {
			TreeTablePath path;
			path.AddComponent(i);
			fFunctionsTable->SetNodeExpanded(path, true, true);
		}
	}

	fFunctionsTable->ResizeAllColumnsToPreferred();
}


// #pragma mark - Listener


ImageFunctionsView::Listener::~Listener()
{
}
