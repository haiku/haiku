/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "StackTraceView.h"

#include <stdio.h>

#include <new>

#include <ControlLook.h>
#include <MessageRunner.h>
#include <Window.h>

#include "table/TableColumns.h"

#include "FunctionInstance.h"
#include "GuiSettingsUtils.h"
#include "Image.h"
#include "StackTrace.h"
#include "TargetAddressTableColumn.h"
#include "UiUtils.h"


enum {
	MSG_CLEAR_STACK_TRACE 	= 'clst'
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


// #pragma mark - StackTraceView


StackTraceView::StackTraceView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fStackTrace(NULL),
	fFramesTable(NULL),
	fFramesTableModel(NULL),
	fTraceUpdateRunner(NULL),
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
	else if (stackTrace == NULL) {
		if (fTraceUpdateRunner == NULL) {
			BMessage message(MSG_CLEAR_STACK_TRACE);
			message.AddPointer("currentTrace", fStackTrace);
			fTraceUpdateRunner = new(std::nothrow) BMessageRunner(this,
				message, 250000, 1);
			if (fTraceUpdateRunner != NULL
				&& fTraceUpdateRunner->InitCheck() != B_OK) {
				delete fTraceUpdateRunner;
				fTraceUpdateRunner = NULL;
			}
		}
	} else {
		delete fTraceUpdateRunner;
		fTraceUpdateRunner = NULL;
		_SetStackTrace(stackTrace);
	}
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
StackTraceView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR_STACK_TRACE:
		{
			StackTrace* currentStackTrace;
			if (message->FindPointer("currentTrace",
					reinterpret_cast<void**>(&currentStackTrace))
					== B_OK && currentStackTrace == fStackTrace) {
				_SetStackTrace(NULL);
			}
			break;
		}
		default:
		{
			BGroupView::MessageReceived(message);
			break;
		}
	}
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


void
StackTraceView::_SetStackTrace(StackTrace* stackTrace)
{
	if (fStackTrace != NULL)
		fStackTrace->ReleaseReference();

	fStackTrace = stackTrace;

	if (fStackTrace != NULL)
		fStackTrace->AcquireReference();

	fFramesTableModel->SetStackTrace(fStackTrace);
}


// #pragma mark - Listener


StackTraceView::Listener::~Listener()
{
}
