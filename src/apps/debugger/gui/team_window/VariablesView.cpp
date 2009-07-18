/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "VariablesView.h"

#include <stdio.h>

#include <new>

#include <AutoLocker.h>

#include "table/TableColumns.h"

#include "Architecture.h"
#include "StackFrame.h"
#include "StackFrameValues.h"
#include "Team.h"
#include "Thread.h"
#include "TypeComponentPath.h"
#include "Variable.h"


// #pragma mark - VariableValueColumn


class VariablesView::VariableValueColumn : public StringTableColumn {
public:
	VariableValueColumn(int32 modelIndex, const char* title, float width,
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
		return StringTableColumn::PrepareField(
			BVariant(_ToString(value, buffer, sizeof(buffer)),
				B_VARIANT_DONT_COPY_DATA));
	}

	virtual int CompareValues(const BVariant& a, const BVariant& b)
	{
		// If neither value is a number, compare the strings. If only one value
		// is a number, it is considered to be greater.
		if (!a.IsNumber()) {
			if (b.IsNumber())
				return -1;
			char bufferA[64];
			char bufferB[64];
			return StringTableColumn::CompareValues(
				BVariant(_ToString(a, bufferA, sizeof(bufferA)),
					B_VARIANT_DONT_COPY_DATA),
				BVariant(_ToString(b, bufferB, sizeof(bufferB)),
					B_VARIANT_DONT_COPY_DATA));
		}

		if (!b.IsNumber())
			return 1;

		// If either value is floating point, we compare floating point values.
		if (a.IsFloat() || b.IsFloat()) {
			double valueA = a.ToDouble();
			double valueB = b.ToDouble();
			return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
		}

		uint64 valueA = a.ToUInt64();
		uint64 valueB = b.ToUInt64();
		return valueA < valueB ? -1 : (valueA == valueB ? 0 : 1);
	}

private:
	const char* _ToString(const BVariant& value, char* buffer,
		size_t bufferSize) const
	{
		switch (value.Type()) {
			case B_BOOL_TYPE:
				return value.ToBool() ? "true" : "false";
			case B_FLOAT_TYPE:
			case B_DOUBLE_TYPE:
				snprintf(buffer, bufferSize, "%g", value.ToDouble());
				break;
			case B_INT8_TYPE:
			case B_UINT8_TYPE:
				snprintf(buffer, bufferSize, "0x%02x", value.ToUInt8());
				break;
			case B_INT16_TYPE:
			case B_UINT16_TYPE:
				snprintf(buffer, bufferSize, "0x%04x", value.ToUInt16());
				break;
			case B_INT32_TYPE:
			case B_UINT32_TYPE:
				snprintf(buffer, bufferSize, "0x%08lx", value.ToUInt32());
				break;
			case B_INT64_TYPE:
			case B_UINT64_TYPE:
				snprintf(buffer, bufferSize, "0x%016llx", value.ToUInt64());
				break;
			case B_STRING_TYPE:
				return value.ToString();
			default:
				return NULL;
		}

		return buffer;
	}
};


// #pragma mark - VariableTableModel


#include "ObjectID.h"
class VariablesView::VariableTableModel : public TableModel {
public:
	VariableTableModel()
		:
		fStackFrame(NULL),
		fValues(NULL)
	{
	}

	~VariableTableModel()
	{
		SetStackFrame(NULL, NULL);
	}

	void SetStackFrame(Thread* thread, StackFrame* stackFrame)
	{
		if (fValues != NULL) {
			fValues->ReleaseReference();
			fValues = NULL;
		}

		if (fStackFrame != NULL) {
			int32 rowCount = CountRows();
			fStackFrame = NULL;
			NotifyRowsRemoved(0, rowCount);
		}

		fStackFrame = stackFrame;
		fThread = thread;

		if (fStackFrame != NULL) {
			try {
				AutoLocker<Team> locker(fThread->GetTeam());
				fValues = new StackFrameValues(*fStackFrame->Values());
			} catch (std::bad_alloc) {
			}

			NotifyRowsAdded(0, CountRows());
		}
	}

	void StackFrameValueRetrieved(StackFrame* stackFrame, Variable* variable,
		TypeComponentPath* path)
	{
		if (stackFrame != fStackFrame || fValues == NULL)
			return;

		// update the respective value
		AutoLocker<Team> locker(fThread->GetTeam());
		BVariant value;
		if (fStackFrame->Values()->GetValue(variable->ID(), path, value)) {
			fValues->SetValue(variable->ID(), path, value);
			NotifyRowsChanged(0, CountRows());
				// TODO: Only notify for the respective node.
		}
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual int32 CountRows() const
	{
		return fStackFrame != NULL
			? fStackFrame->CountParameters()
				+ fStackFrame->CountLocalVariables()
			: 0;
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& _value)
	{
		if (fStackFrame == NULL)
			return false;

		int32 parameterCount = fStackFrame->CountParameters();
		const Variable* variable = rowIndex < parameterCount
			? fStackFrame->ParameterAt(rowIndex)
			: fStackFrame->LocalVariableAt(rowIndex - parameterCount);
		if (variable == NULL)
			return false;
printf("VariablesView::VariableTableModel::GetValueAt(%ld, %ld): "
"variable id: %p, hash: %lu\n", rowIndex, columnIndex, variable->ID(),
variable->ID()->HashValue());

		switch (columnIndex) {
			case 0:
				_value.SetTo(variable->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				if (fValues == NULL)
					return false;
//				return fValues->GetValue(variable->ID(), TypeComponentPath(),
//					_value);
{
bool success = fValues->GetValue(variable->ID(), TypeComponentPath(), _value);
if (!success)
	return false;
printf("  -> %llx\n", _value.ToUInt64());
return true;
}
			default:
				return false;
		}
	}

private:
	Thread*				fThread;
	StackFrame*			fStackFrame;
	StackFrameValues*	fValues;
};


// #pragma mark - VariablesView


VariablesView::VariablesView(Listener* listener)
	:
	BGroupView(B_VERTICAL),
	fThread(NULL),
	fStackFrame(NULL),
	fVariableTable(NULL),
	fVariableTableModel(NULL),
	fListener(listener)
{
	SetName("Variables");
}


VariablesView::~VariablesView()
{
	SetStackFrame(NULL, NULL);
	fVariableTable->SetTableModel(NULL);
	delete fVariableTableModel;
}


/*static*/ VariablesView*
VariablesView::Create(Listener* listener)
{
	VariablesView* self = new VariablesView(listener);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
VariablesView::SetStackFrame(Thread* thread, StackFrame* stackFrame)
{
	if (thread == fThread && stackFrame == fStackFrame)
		return;

	if (fThread != NULL)
		fThread->ReleaseReference();
	if (fStackFrame != NULL)
		fStackFrame->ReleaseReference();

	fThread = thread;
	fStackFrame = stackFrame;

	if (fThread != NULL)
		fThread->AcquireReference();
	if (fStackFrame != NULL)
		fStackFrame->AcquireReference();

	fVariableTableModel->SetStackFrame(fThread, fStackFrame);

	// request loading the parameter and variable values
	if (fThread != NULL && fStackFrame != NULL) {
		AutoLocker<Team> locker(fThread->GetTeam());

		for (int32 i = 0; Variable* variable = fStackFrame->ParameterAt(i); i++)
			_RequestVariableValue(variable);

		for (int32 i = 0; Variable* variable = fStackFrame->LocalVariableAt(i);
				i++) {
			_RequestVariableValue(variable);
		}
	}
}


void
VariablesView::StackFrameValueRetrieved(StackFrame* stackFrame,
	Variable* variable, TypeComponentPath* path)
{
	fVariableTableModel->StackFrameValueRetrieved(stackFrame, variable, path);
}


void
VariablesView::_Init()
{
	fVariableTable = new Table("variable list", 0, B_FANCY_BORDER);
	AddChild(fVariableTable->ToView());

	// columns
	fVariableTable->AddColumn(new StringTableColumn(0, "Variable", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fVariableTable->AddColumn(new VariableValueColumn(1, "Value", 80, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));

	fVariableTableModel = new VariableTableModel;
	fVariableTable->SetTableModel(fVariableTableModel);

	fVariableTable->AddTableListener(this);
}


void
VariablesView::_RequestVariableValue(Variable* variable)
{
	StackFrameValues* values = fStackFrame->Values();
	if (values->HasValue(variable->ID(), TypeComponentPath()))
		return;

	TypeComponentPath* path = new(std::nothrow) TypeComponentPath;
	if (path == NULL)
		return;
	Reference<TypeComponentPath> pathReference(path, true);

	fListener->StackFrameValueRequested(fThread, fStackFrame,
		variable, path);
}


// #pragma mark - Listener


VariablesView::Listener::~Listener()
{
}
