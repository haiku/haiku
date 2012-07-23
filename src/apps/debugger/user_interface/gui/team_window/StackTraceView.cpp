/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "StackTraceView.h"

#include <stdio.h>

#include <new>

#include <ControlLook.h>

#include "table/TableColumns.h"

#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "Image.h"
#include "StackTrace.h"
#include "TargetAddressTableColumn.h"


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
				Image* image = frame->GetImage();
				FunctionInstance* function = frame->Function();
				if (image == NULL && function == NULL) {
					value.SetTo("?", B_VARIANT_DONT_COPY_DATA);
					return true;
				}

				BString name;
				target_addr_t baseAddress;
				if (function != NULL) {
					name = function->PrettyName();
					baseAddress = function->Address();
				} else {
					name = image->Name();
					baseAddress = image->Info().TextBase();
				}

				char offset[32];
				snprintf(offset, sizeof(offset), " + %#llx",
					frame->InstructionPointer() - baseAddress);

				name << offset;
				value.SetTo(name.String());

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


// #pragma mark - StackTraceView


StackTraceView::StackTraceView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fStackTrace(NULL),
	fFramesTable(NULL),
	fFramesTableModel(NULL),
	fListener(listener)
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
		for (int32 i = 0; StackFrame* other = fStackTrace->FrameAt(i); i++) {
			if (stackFrame == other) {
				fFramesTable->SelectRow(i, false);
				return;
			}
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
StackTraceView::TableSelectionChanged(Table* table)
{
	if (fListener == NULL)
		return;

	StackFrame* frame
		= fFramesTableModel->FrameAt(table->SelectionModel()->RowAt(0));

	fListener->StackFrameSelectionChanged(frame);
}


void
StackTraceView::_Init()
{
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
