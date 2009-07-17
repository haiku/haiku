/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesView.h"

#include <stdio.h>

#include <new>

#include "table/TableColumns.h"

#include "Architecture.h"
#include "StackFrame.h"
#include "Variable.h"


// #pragma mark - VariableTableModel


class VariablesView::VariableTableModel : public TableModel {
public:
	VariableTableModel()
		:
		fStackFrame(NULL)
	{
	}

	~VariableTableModel()
	{
	}

	void SetStackFrame(StackFrame* stackFrame)
	{
		if (fStackFrame != NULL) {
			int32 rowCount = CountRows();
			fStackFrame = NULL;
			NotifyRowsRemoved(0, rowCount);
		}

		fStackFrame = stackFrame;

		if (fStackFrame != NULL)
			NotifyRowsAdded(0, CountRows());
	}

	virtual int32 CountColumns() const
	{
		return 1;
	}

	virtual int32 CountRows() const
	{
		return fStackFrame != NULL
			? fStackFrame->CountParameters()
				+ fStackFrame->CountLocalVariables()
			: 0;
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		if (fStackFrame == NULL)
			return false;

		int32 parameterCount = fStackFrame->CountParameters();
		const Variable* variable = rowIndex < parameterCount
			? fStackFrame->ParameterAt(rowIndex)
			: fStackFrame->LocalVariableAt(rowIndex - parameterCount);
		if (variable == NULL)
			return false;

		switch (columnIndex) {
			case 0:
				value.SetTo(variable->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			default:
				return false;
		}
	}

private:
	StackFrame*		fStackFrame;
};


// #pragma mark - VariablesView


VariablesView::VariablesView()
	:
	BGroupView(B_VERTICAL),
	fStackFrame(NULL),
	fVariableTable(NULL),
	fVariableTableModel(NULL)
{
	SetName("Variables");
}


VariablesView::~VariablesView()
{
	SetStackFrame(NULL);
	fVariableTable->SetTableModel(NULL);
	delete fVariableTableModel;
}


/*static*/ VariablesView*
VariablesView::Create()
{
	VariablesView* self = new VariablesView;

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
VariablesView::SetStackFrame(StackFrame* stackFrame)
{
	if (stackFrame == fStackFrame)
		return;

	if (fStackFrame != NULL)
		fStackFrame->RemoveReference();

	fStackFrame = stackFrame;

	if (fStackFrame != NULL)
		fStackFrame->AddReference();

	fVariableTableModel->SetStackFrame(fStackFrame);
}


void
VariablesView::_Init()
{
	fVariableTable = new Table("variable list", 0, B_FANCY_BORDER);
	AddChild(fVariableTable->ToView());

	// columns
	fVariableTable->AddColumn(new StringTableColumn(0, "Variable", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
//	fVariableTable->AddColumn(new VariableValueColumn(1, "Value", 80, 40, 1000,
//		B_TRUNCATE_END, B_ALIGN_RIGHT));

	fVariableTableModel = new VariableTableModel;
	fVariableTable->SetTableModel(fVariableTableModel);

	fVariableTable->AddTableListener(this);
}
