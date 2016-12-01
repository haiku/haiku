/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011-2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "RegistersView.h"

#include <stdio.h>

#include <new>

#include <ControlLook.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "table/TableColumns.h"

#include "AppMessageCodes.h"
#include "Architecture.h"
#include "AutoDeleter.h"
#include "CpuState.h"
#include "GuiSettingsUtils.h"
#include "Register.h"
#include "UiUtils.h"


enum {
	MSG_SIMD_RENDER_FORMAT_CHANGED = 'srfc'
};


static const char*
GetLabelForSIMDFormat(int format)
{
	switch (format) {
			case SIMD_RENDER_FORMAT_INT8:
				return "8-bit integer";
			case SIMD_RENDER_FORMAT_INT16:
				return "16-bit integer";
			case SIMD_RENDER_FORMAT_INT32:
				return "32-bit integer";
			case SIMD_RENDER_FORMAT_INT64:
				return "64-bit integer";
			case SIMD_RENDER_FORMAT_FLOAT:
				return "Float";
			case SIMD_RENDER_FORMAT_DOUBLE:
				return "Double";
	}

	return "Unknown";
}


// #pragma mark - RegisterValueColumn


class RegistersView::RegisterValueColumn : public StringTableColumn {
public:
	RegisterValueColumn(int32 modelIndex, const char* title, float width,
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
			BVariant(UiUtils::VariantToString(value, buffer, sizeof(buffer)),
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
				BVariant(UiUtils::VariantToString(a, bufferA,
						sizeof(bufferA)),
					B_VARIANT_DONT_COPY_DATA),
				BVariant(UiUtils::VariantToString(b, bufferB,
						sizeof(bufferB)),
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
};


// #pragma mark - RegisterTableModel


class RegistersView::RegisterTableModel : public TableModel {
public:
	RegisterTableModel(Architecture* architecture)
		:
		fArchitecture(architecture),
		fCpuState(NULL),
		fSIMDFormat(SIMD_RENDER_FORMAT_INT16)
	{
	}

	~RegisterTableModel()
	{
	}

	void SetCpuState(CpuState* cpuState)
	{
		fCpuState = cpuState;

		NotifyRowsChanged(0, CountRows());
	}

	virtual int32 CountColumns() const
	{
		return 2;
	}

	virtual int32 CountRows() const
	{
		return fArchitecture->CountRegisters();
	}

	inline int32 SIMDRenderFormat() const
	{
		return fSIMDFormat;
	}

	virtual bool GetValueAt(int32 rowIndex, int32 columnIndex, BVariant& value)
	{
		if (rowIndex < 0 || rowIndex >= fArchitecture->CountRegisters())
			return false;

		const Register* reg = fArchitecture->Registers() + rowIndex;

		switch (columnIndex) {
			case 0:
				value.SetTo(reg->Name(), B_VARIANT_DONT_COPY_DATA);
				return true;
			case 1:
				if (fCpuState == NULL)
					return false;
				if (!fCpuState->GetRegisterValue(reg, value))
					value.SetTo("?", B_VARIANT_DONT_COPY_DATA);
				else if (reg->Format() == REGISTER_FORMAT_SIMD) {
					BString output;
					value.SetTo(UiUtils::FormatSIMDValue(value,
						reg->BitSize(),fSIMDFormat, output));
				}
				return true;
			default:
				return false;
		}
	}

	void SetSIMDFormat(int32 format)
	{
		if (fSIMDFormat != format) {
			fSIMDFormat = format;
			NotifyRowsChanged(0, CountRows());
		}
	}

private:
private:
	Architecture*	fArchitecture;
	CpuState*		fCpuState;
	int32			fSIMDFormat;
};


// #pragma mark - RegistersView


RegistersView::RegistersView(Architecture* architecture)
	:
	BGroupView(B_VERTICAL),
	fArchitecture(architecture),
	fCpuState(NULL),
	fRegisterTable(NULL),
	fRegisterTableModel(NULL)
{
	SetName("Registers");
}


RegistersView::~RegistersView()
{
	SetCpuState(NULL);
	fRegisterTable->SetTableModel(NULL);
	delete fRegisterTableModel;
}


/*static*/ RegistersView*
RegistersView::Create(Architecture* architecture)
{
	RegistersView* self = new RegistersView(architecture);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}


void
RegistersView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SIMD_RENDER_FORMAT_CHANGED:
			{
				int32 format;
				if (message->FindInt32("format", &format) != B_OK)
					break;

				fRegisterTableModel->SetSIMDFormat(format);
			}
			break;

		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
RegistersView::SetCpuState(CpuState* cpuState)
{
	if (cpuState == fCpuState)
		return;

	if (fCpuState != NULL)
		fCpuState->ReleaseReference();

	fCpuState = cpuState;

	if (fCpuState != NULL)
		fCpuState->AcquireReference();

	fRegisterTableModel->SetCpuState(fCpuState);
}


void
RegistersView::LoadSettings(const BMessage& settings)
{
	BMessage tableSettings;
	if (settings.FindMessage("registerTable", &tableSettings) == B_OK) {
		GuiSettingsUtils::UnarchiveTableSettings(tableSettings,
			fRegisterTable);
	}
}


status_t
RegistersView::SaveSettings(BMessage& settings)
{
	settings.MakeEmpty();

	BMessage tableSettings;
	status_t result = GuiSettingsUtils::ArchiveTableSettings(tableSettings,
		fRegisterTable);
	if (result == B_OK)
		result = settings.AddMessage("registerTable", &tableSettings);

	return result;
}


void
RegistersView::TableRowInvoked(Table* table, int32 rowIndex)
{
}


void
RegistersView::TableCellMouseDown(Table* table, int32 rowIndex,
	int32 columnIndex, BPoint screenWhere,	uint32 buttons)
{
	if (rowIndex < 0 || rowIndex >= fArchitecture->CountRegisters())
		return;

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) == 0)
		return;

	BVariant value;
	if (!fRegisterTableModel->GetValueAt(rowIndex, 1, value))
		return;

	const Register* reg = fArchitecture->Registers() + rowIndex;
	if (reg->Format() == REGISTER_FORMAT_FLOAT) {
		// for floating point registers, we currently have no
		// context menu options to display.
		return;
	}

	BPopUpMenu* menu = new(std::nothrow) BPopUpMenu("Options");
	if (menu == NULL)
		return;

	ObjectDeleter<BPopUpMenu> menuDeleter(menu);

	if (reg->Format() == REGISTER_FORMAT_INTEGER) {
		BMessage* message = new(std::nothrow) BMessage(MSG_SHOW_INSPECTOR_WINDOW);
		if (message == NULL)
			return;

		message->AddUInt64("address", value.ToUInt64());

		ObjectDeleter<BMessage> messageDeleter(message);
		BMenuItem* item = new(std::nothrow) BMenuItem("Inspect", message);
		if (item == NULL)
			return;

		messageDeleter.Detach();
		ObjectDeleter<BMenuItem> itemDeleter(item);
		if (!menu->AddItem(item))
			return;

		itemDeleter.Detach();

		item->SetTarget(Window());
	} else if (reg->Format() == REGISTER_FORMAT_SIMD) {
		BMenu* formatMenu = new(std::nothrow) BMenu("Format");
		if (formatMenu == NULL)
			return;

		ObjectDeleter<BMenu> formatMenuDeleter(formatMenu);
		if (!menu->AddItem(formatMenu))
			return;
		formatMenuDeleter.Detach();

		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_INT8) != B_OK)
			return;
		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_INT16) != B_OK)
			return;
		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_INT32) != B_OK)
			return;
		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_INT64) != B_OK)
			return;
		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_FLOAT) != B_OK)
			return;
		if (_AddFormatItem(formatMenu, SIMD_RENDER_FORMAT_DOUBLE) != B_OK)
			return;

		formatMenu->SetTargetForItems(this);
	}

	menuDeleter.Detach();

	BRect mouseRect(screenWhere, screenWhere);
	mouseRect.InsetBy(-4.0, -4.0);
	menu->Go(screenWhere, true, false, mouseRect, true);
}


void
RegistersView::_Init()
{
	fRegisterTable = new Table("register list", 0, B_FANCY_BORDER);
	AddChild(fRegisterTable->ToView());

	// columns
	fRegisterTable->AddColumn(new StringTableColumn(0, "Register",
		be_plain_font->StringWidth("Register")
			+ be_control_look->DefaultLabelSpacing() * 2 + 5, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_LEFT));
	fRegisterTable->AddColumn(new RegisterValueColumn(1, "Value",
		be_plain_font->StringWidth("0x00000000")
			+ be_control_look->DefaultLabelSpacing() * 2 + 5, 40, 1000,
		B_TRUNCATE_END, B_ALIGN_RIGHT));

	fRegisterTableModel = new RegisterTableModel(fArchitecture);
	fRegisterTable->SetTableModel(fRegisterTableModel);

	fRegisterTable->AddTableListener(this);
}


status_t
RegistersView::_AddFormatItem(BMenu* menu, int32 format)
{
	BMessage* message = new(std::nothrow) BMessage(
		MSG_SIMD_RENDER_FORMAT_CHANGED);
	if (message == NULL)
		return B_NO_MEMORY;

	ObjectDeleter<BMessage> messageDeleter(message);
	if (message->AddInt32("format", format) != B_OK)
		return B_NO_MEMORY;

	BMenuItem* item = new(std::nothrow) BMenuItem(
		GetLabelForSIMDFormat(format), message);
	if (item == NULL)
		return B_NO_MEMORY;

	messageDeleter.Detach();
	ObjectDeleter<BMenuItem> itemDeleter(item);
	if (!menu->AddItem(item))
		return B_NO_MEMORY;

	itemDeleter.Detach();

	if (format == fRegisterTableModel->SIMDRenderFormat())
		item->SetMarked(true);

	return B_OK;
}


