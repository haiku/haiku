/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "StackTraceView.h"

#include <stdio.h>

#include <new>

#include <ControlLook.h>
#include <Window.h>

#include <AutoDeleter.h>

#include "table/TableColumns.h"

#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "Image.h"
#include "StackTrace.h"
#include "TargetAddressTableColumn.h"
#include "UiUtils.h"


// #pragma mark - FramesTableModel


class StackTraceView::FramesTableModel : public TableModel {
public:
	FramesTableModel()
		:
		fStackTrace(NULL)
	{
	}

	~FramesTableModel()
	{
		SetStackTrace(NULL);
	}

	void SetStackTrace(StackTrace* stackTrace)
	{
		// unset old frames
		if (fStackTrace != NULL && fStackTrace->CountFrames())
			NotifyRowsRemoved(0, fStackTrace->CountFrames());

		fStackTrace = stackTrace;

		// set new frames
		if (fStackTrace != NULL && fStackTrace->CountFrames() > 0)
			NotifyRowsAdded(0, fStackTrace->CountFrames());
	}

	virtual int32 CountColumns() const
	{
		return 3;
	}

	virtual int32 CountRows() const
	{
		return fStackTrace != NULL ? fStackTrace->CountFrames() : 0;
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		StackFrame* frame
			= fStackTrace != NULL ? fStackTrace->FrameAt(rowIndex) : NULL;
		if (frame == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(frame->FrameAddress());
				return true;
			case 1:
				value.SetTo(frame->InstructionPointer());
				return true;
			case 2:
			{
				char buffer[512];
				value.SetTo(UiUtils::FunctionNameForFrame(frame, buffer,
						sizeof(buffer)));
				return true;
			}
			default:
				return false;
		}
	}

	StackFrame* FrameAt(int32 index) const
	{
		return fStackTrace != NULL ? fStackTrace->FrameAt(index) : NULL;
	}

private:
	StackTrace*				fStackTrace;
};


// #pragma mark - StackTraceKey


struct StackTraceView::StackTraceKey {
	StackTrace*			stackTrace;

	StackTraceKey(StackTrace* stackTrace)
		:
		stackTrace(stackTrace)
	{
	}

	uint32 HashValue() const
	{
		return *(uint32*)stackTrace;
	}

	bool operator==(const StackTraceKey& other) const
	{
		return stackTrace == other.stackTrace;
	}
};


// #pragma mark - StackTraceSelectionEntry


struct StackTraceView::StackTraceSelectionEntry : StackTraceKey {
	StackTraceSelectionEntry* next;
	int32 selectedFrameIndex;

	StackTraceSelectionEntry(StackTrace* stackTrace)
		:
		StackTraceKey(stackTrace),
		selectedFrameIndex(0)
	{
	}

	inline int32 SelectedFrameIndex() const
	{
		return selectedFrameIndex;
	}

	void SetSelectedFrameIndex(int32 index)
	{
		selectedFrameIndex = index;
	}
};


// #pragma mark - StackTraceSelectionEntryHashDefinition


struct StackTraceView::StackTraceSelectionEntryHashDefinition {
	typedef StackTraceKey				KeyType;
	typedef	StackTraceSelectionEntry	ValueType;

	size_t HashKey(const StackTraceKey& key) const
	{
		return key.HashValue();
	}

	size_t Hash(const StackTraceSelectionEntry* value) const
	{
		return value->HashValue();
	}

	bool Compare(const StackTraceKey& key,
		const StackTraceSelectionEntry* value) const
	{
		return key == *value;
	}

	StackTraceSelectionEntry*& GetLink(StackTraceSelectionEntry* value) const
	{
		return value->next;
	}
};


// #pragma mark - StackTraceView


StackTraceView::StackTraceView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fStackTrace(NULL),
	fFramesTable(NULL),
	fFramesTableModel(NULL),
	fTraceClearPending(false),
	fSelectionInfoTable(NULL),
	fListener(listener)
{
	SetName("Stack Trace");
}


StackTraceView::~StackTraceView()
{
	SetStackTrace(NULL);
	fFramesTable->SetTableModel(NULL);
	delete fFramesTableModel;
	delete fSelectionInfoTable;
}


/*static*/ StackTraceView*
StackTraceView::Create(Listener* listener)
{
	StackTraceView* self = new StackTraceView(listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
StackTraceView::UnsetListener()
{
	fListener = NULL;
}


void
StackTraceView::SetStackTrace(StackTrace* stackTrace)
{
	fTraceClearPending = false;
	if (stackTrace == fStackTrace)
		return;

	if (fStackTrace != NULL)
		fStackTrace->ReleaseReference();

	fStackTrace = stackTrace;

	if (fStackTrace != NULL)
		fStackTrace->AcquireReference();

	fFramesTableModel->SetStackTrace(fStackTrace);
}


void
StackTraceView::SetStackFrame(StackFrame* stackFrame)
{
	if (fStackTrace != NULL && stackFrame != NULL) {
		int32 selectedIndex = -1;
		StackTraceSelectionEntry* entry = fSelectionInfoTable->Lookup(
			fStackTrace);
		if (entry != NULL)
			selectedIndex = entry->SelectedFrameIndex();
		else {
			for (int32 i = 0; StackFrame* other = fStackTrace->FrameAt(i);
				i++) {
				if (stackFrame == other) {
					selectedIndex = i;
					break;
				}
			}
		}

		if (selectedIndex >= 0) {
			fFramesTable->SelectRow(selectedIndex, false);
			return;
		}
	}

	fFramesTable->DeselectAllRows();
}


void
StackTraceView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("framesTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fFramesTable);
	}
}


status_t
StackTraceView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
		fFramesTable);
	if (result == B_OK)
		result = settings.AddMessage("framesTable", &tableSettings);

	return result;
}


void
StackTraceView::SetStackTraceClearPending()
{
	fTraceClearPending = true;
	StackTraceSelectionEntry* entry = fSelectionInfoTable->Lookup(fStackTrace);
	if (entry != NULL) {
		fSelectionInfoTable->Remove(entry);
		delete entry;
	}
}


void
StackTraceView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL || fTraceClearPending)
		return;

	int32 selectedIndex = table->SelectionModel()->RowAt(0);
	StackFrame* frame = fFramesTableModel->FrameAt(selectedIndex);

	StackTraceSelectionEntry* entry = fSelectionInfoTable->Lookup(fStackTrace);
	if (entry == NULL) {
		entry = new(std::nothrow) StackTraceSelectionEntry(fStackTrace);
		if (entry == NULL)
			return;

		ObjectDeleter<StackTraceSelectionEntry> entryDeleter(entry);
		if (fSelectionInfoTable->Insert(entry) != B_OK)
			return;

		entryDeleter.Detach();
	}

	entry->SetSelectedFrameIndex(selectedIndex);

	fListener->StackFrameSelectionChanged(frame);
}


void
StackTraceView::_Init()
{
	fSelectionInfoTable = new StackTraceSelectionInfoTable;
	if (fSelectionInfoTable->Init() != B_OK) {
		delete fSelectionInfoTable;
		fSelectionInfoTable = NULL;
		throw std::bad_alloc();
	}

	fFramesTable = new Table("stack trace", 0, B_FANCY_BORDER);
	AddChild(fFramesTable->ToView());
	fFramesTable->SetSortingEnabled(false);

	float addressWidth = be_plain_font->StringWidth("0x00000000")
		+ be_control_look->DefaultLabelSpacing() * 2 + 5;

	// columns
	fFramesTable->AddColumn(new TargetAddressTableColumn(0, "Frame",
		addressWidth, 40, 1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fFramesTable->AddColumn(new TargetAddressTableColumn(1, "IP", addressWidth,
		40, 1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fFramesTable->AddColumn(new StringTableColumn(2, "Function", 300, 100, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fFramesTableModel = new FramesTableModel();
	fFramesTable->SetTableModel(fFramesTableModel);

	fFramesTable->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fFramesTable->AddTableListener(this);
}


// #pragma mark - Listener


StackTraceView::Listener::~Listener()
{
}
