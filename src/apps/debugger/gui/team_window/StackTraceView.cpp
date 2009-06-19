/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "StackTraceView.h"

#include <stdio.h>

#include <new>

#include "table/TableColumns.h"

#include "StackTrace.h"


// #pragma mark - TargetAddressValueColumn


class TargetAddressValueColumn : public StringTableColumn {
public:
	TargetAddressValueColumn(int32 modelIndex, const char* title, float width,
		float minWidth, float maxWidth, uint32 truncate = B_TRUNCATE_MIDDLE,
		alignment align = B_ALIGN_RIGHT)
		:
		StringTableColumn(modelIndex, title, width, minWidth, maxWidth,
			truncate, align)
	{
	}

protected:
	virtual BField* PrepareField(const BVariant& value) const
	{
		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%#llx", value.ToUInt64());

		return StringTableColumn::PrepareField(
			BVariant(buffer, B_VARIANT_DONT_COPY_DATA));
	}

	virtual int CompareValues(const BVariant& a, const BVariant& b)
	{
		uint64 valueA = a.ToUInt64();
		uint64 valueB = b.ToUInt64();
		return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
	}
};


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
		if (fFrames.CountItems() > 0) {
			NotifyRowsRemoved(0, fFrames.CountItems());

			for (int32 i = 0; StackFrame* frame = fFrames.ItemAt(i); i++)
				frame->RemoveReference();

			fFrames.MakeEmpty();
		}

		fStackTrace = stackTrace;

		// set new frames
		if (fStackTrace != NULL) {
			for (StackFrameList::ConstIterator it
						= fStackTrace->Frames().GetIterator();
					StackFrame* frame = it.Next();) {
				if (!fFrames.AddItem(frame))
					return;
				frame->AddReference();
			}

			if (fFrames.CountItems() > 0)
				NotifyRowsAdded(0, fFrames.CountItems());
		}
	}

	virtual int32 CountColumns() const
	{
		return 3;
	}

	virtual int32 CountRows() const
	{
		return fFrames.CountItems();
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		StackFrame* frame = fFrames.ItemAt(rowIndex);
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
				// TODO: function name...
				return false;
			default:
				return false;
		}
	}

private:
	StackTrace*				fStackTrace;
	BObjectList<StackFrame>	fFrames;
};


// #pragma mark - StackTraceView


StackTraceView::StackTraceView()
	:
	BGroupView(B_VERTICAL),
	fFramesTable(NULL),
	fFramesTableModel(NULL)
{
	SetName("Stack Trace");
}


StackTraceView::~StackTraceView()
{
	SetStackTrace(NULL);
	fFramesTable->SetTableModel(NULL);
	delete fFramesTableModel;
}


/*static*/ StackTraceView*
StackTraceView::Create()
{
	StackTraceView* self = new StackTraceView();

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
StackTraceView::SetStackTrace(StackTrace* stackTrace)
{
	if (stackTrace == fStackTrace)
		return;

	if (fStackTrace != NULL)
		fStackTrace->RemoveReference();

	fStackTrace = stackTrace;

	if (fStackTrace != NULL)
		fStackTrace->AddReference();

	fFramesTableModel->SetStackTrace(fStackTrace);
}


void
StackTraceView::TableRowInvoked(Table* table, int32 rowIndex)
{
}


void
StackTraceView::_Init()
{
	fFramesTable = new Table("register list", 0);
	AddChild(fFramesTable->ToView());
	fFramesTable->SetSortingEnabled(false);

	// columns
	fFramesTable->AddColumn(new TargetAddressValueColumn(0, "Frame", 80, 40,
		1000, B_TRUNCATE_END, B_ALIGN_RIGHT));
	fFramesTable->AddColumn(new TargetAddressValueColumn(1, "IP", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));
	fFramesTable->AddColumn(new StringTableColumn(2, "Function", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));

	fFramesTableModel = new FramesTableModel();
	fFramesTable->SetTableModel(fFramesTableModel);

	fFramesTable->AddTableListener(this);
}
