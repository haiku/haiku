/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */


#include "ExtendedInfoWindow.h"

#include <Box.h>
#include <Catalog.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PowerStatus"


const int kLineSpacing = 5;


FontString::FontString()
{
	font = be_plain_font;
}


//	#pragma mark -


BatteryInfoView::BatteryInfoView()
	:
	BView("battery info view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(200, 200),
	fMaxStringSize(0, 0)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


BatteryInfoView::~BatteryInfoView()
{
	_ClearStringList();
}


void
BatteryInfoView::Update(battery_info& info, acpi_extended_battery_info& extInfo)
{
	fBatteryInfo = info;
	fBatteryExtendedInfo = extInfo;

	_FillStringList();
}


void
BatteryInfoView::Draw(BRect updateRect)
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BPoint point(10, 10);

	float space = _MeasureString("").height + kLineSpacing;

	for (int i = 0; i < fStringList.CountItems(); i++) {
		FontString* fontString = fStringList.ItemAt(i);
		SetFont(fontString->font);
		DrawString(fontString->string.String(), point);
		point.y += space;
	}
}


void
BatteryInfoView::GetPreferredSize(float* width, float* height)
{
	*width = fPreferredSize.width;
	*height = fPreferredSize.height;
}


void
BatteryInfoView::AttachedToWindow()
{
	Window()->CenterOnScreen();
}


BSize
BatteryInfoView::_MeasureString(const BString& string)
{
	BFont font;
	GetFont(&font);
	BSize size;

	size.width = font.StringWidth(string);

	font_height height;
	font.GetHeight(&height);
	size.height = height.ascent + height.descent;

	return size;
}


void
BatteryInfoView::_FillStringList()
{
	_ClearStringList();

	BString powerUnit;
	BString rateUnit;
	switch (fBatteryExtendedInfo.power_unit) {
		case 0:
			powerUnit = B_TRANSLATE(" mWh");
			rateUnit = B_TRANSLATE(" mW");
			break;

		case 1:
			powerUnit = B_TRANSLATE(" mAh");
			rateUnit = B_TRANSLATE(" mA");
			break;
	}

	FontString* fontString;

	fontString = new FontString;
	fStringList.AddItem(fontString);
	fontString->font = be_bold_font;

	if ((fBatteryInfo.state & BATTERY_CHARGING) != 0)
		fontString->string = B_TRANSLATE("Battery charging");
	else if ((fBatteryInfo.state & BATTERY_DISCHARGING) != 0)
		fontString->string = B_TRANSLATE("Battery discharging");
	else if ((fBatteryInfo.state & BATTERY_CRITICAL_STATE) != 0
		&& fBatteryExtendedInfo.model_number[0] == '\0'
		&& fBatteryExtendedInfo.serial_number[0] == '\0'
		&& fBatteryExtendedInfo.type[0] == '\0'
		&& fBatteryExtendedInfo.oem_info[0] == '\0')
		fontString->string = B_TRANSLATE("Empty battery slot");
	else if ((fBatteryInfo.state & BATTERY_CRITICAL_STATE) != 0)
		fontString->string = B_TRANSLATE("Damaged battery");
	else
		fontString->string = B_TRANSLATE("Battery unused");

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Capacity: ");
	fontString->string << fBatteryInfo.capacity;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Last full charge: ");
	fontString->string << fBatteryInfo.full_capacity;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Current rate: ");
	fontString->string << fBatteryInfo.current_rate;
	fontString->string << rateUnit;
	_AddToStringList(fontString);

	// empty line
	fontString = new FontString;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Design capacity: ");
	fontString->string << fBatteryExtendedInfo.design_capacity;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Technology: ");
	if (fBatteryExtendedInfo.technology == 0)
		fontString->string << B_TRANSLATE("non-rechargeable");
	else if (fBatteryExtendedInfo.technology == 1)
		fontString->string << B_TRANSLATE("rechargeable");
	else
		fontString->string << "?";
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Design voltage: ");
	fontString->string << fBatteryExtendedInfo.design_voltage;
	fontString->string << B_TRANSLATE(" mV");
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Design capacity warning: ");
	fontString->string << fBatteryExtendedInfo.design_capacity_warning;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Design capacity low warning: ");
	fontString->string << fBatteryExtendedInfo.design_capacity_low;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Capacity granularity 1: ");
	fontString->string << fBatteryExtendedInfo.capacity_granularity_1;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Capacity granularity 2: ");
	fontString->string << fBatteryExtendedInfo.capacity_granularity_2;
	fontString->string << powerUnit;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Model number: ");
	fontString->string << fBatteryExtendedInfo.model_number;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Serial number: ");
	fontString->string << fBatteryExtendedInfo.serial_number;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("Type: ");
	fontString->string += fBatteryExtendedInfo.type;
	_AddToStringList(fontString);

	fontString = new FontString;
	fontString->string = B_TRANSLATE("OEM info: ");
	fontString->string += fBatteryExtendedInfo.oem_info;
	_AddToStringList(fontString);

	fPreferredSize.width = fMaxStringSize.width + 10;
	fPreferredSize.height = (fMaxStringSize.height + kLineSpacing) *
		fStringList.CountItems();
}


void
BatteryInfoView::_AddToStringList(FontString* fontString)
{
	fStringList.AddItem(fontString);
	BSize stringSize = _MeasureString(fontString->string);
	if (fMaxStringSize.width < stringSize.width)
		fMaxStringSize = stringSize;
}


void
BatteryInfoView::_ClearStringList()
{
	for (int i = 0; i < fStringList.CountItems(); i ++)
		delete fStringList.ItemAt(i);
	fStringList.MakeEmpty();
	fMaxStringSize = BSize(0, 0);
}


//	#pragma mark -


ExtPowerStatusView::ExtPowerStatusView(PowerStatusDriverInterface* interface,
		BRect frame, int32 resizingMode, int batteryID,
		ExtendedInfoWindow* window)
	:
	PowerStatusView(interface, frame, resizingMode, batteryID),
	fExtendedInfoWindow(window),
	fBatteryInfoView(window->GetExtendedBatteryInfoView()),
	fSelected(false)
{
}


void
ExtPowerStatusView::Draw(BRect updateRect)
{
	if (fSelected)
		SetLowColor(102, 152, 203);
	else
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	PowerStatusView::Draw(updateRect);
}


void
ExtPowerStatusView::MouseDown(BPoint where)
{
	if (!fSelected) {
		fSelected = true;
		Update(true);
		if (ExtendedInfoWindow* window
				= dynamic_cast<ExtendedInfoWindow*>(Window()))
			window->BatterySelected(this);
	}
}


void
ExtPowerStatusView::Select(bool select)
{
	fSelected = select;
	Update(true);
}


bool
ExtPowerStatusView::IsCritical()
{
	return (fBatteryInfo.state & BATTERY_CRITICAL_STATE) != 0;
}


void
ExtPowerStatusView::Update(bool force)
{
	PowerStatusView::Update(force);
	if (!fSelected)
		return;

	acpi_extended_battery_info extInfo;
	fDriverInterface->GetExtendedBatteryInfo(&extInfo, fBatteryID);

	fBatteryInfoView->Update(fBatteryInfo, extInfo);
	fBatteryInfoView->Invalidate();
}


//	#pragma mark -


ExtendedInfoWindow::ExtendedInfoWindow(PowerStatusDriverInterface* interface)
	:
	BWindow(BRect(100, 150, 500, 500), B_TRANSLATE("Extended battery info"),
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AVOID_FRONT |
		B_ASYNCHRONOUS_CONTROLS),
	fDriverInterface(interface),
	fSelectedView(NULL)
{
	fDriverInterface->AcquireReference();

	BView *view = new BView(Bounds(), "view", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
	mainLayout->SetSpacing(10);
	mainLayout->SetInsets(10, 10, 10, 10);
	view->SetLayout(mainLayout);

	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	BBox *infoBox = new BBox(rect, B_TRANSLATE("Power status box"));
	infoBox->SetLabel(B_TRANSLATE("Battery info"));
	BGroupLayout* infoLayout = new BGroupLayout(B_HORIZONTAL);
	infoLayout->SetInsets(10, infoBox->TopBorderOffset() * 2 + 10, 10, 10);
	infoLayout->SetSpacing(10);
	infoBox->SetLayout(infoLayout);
	mainLayout->AddView(infoBox);

	BGroupView* batteryView = new BGroupView(B_VERTICAL);
	batteryView->GroupLayout()->SetSpacing(10);
	infoLayout->AddView(batteryView);

	// create before the battery views
	fBatteryInfoView = new BatteryInfoView();

	BGroupLayout* batteryLayout = batteryView->GroupLayout();
	BRect batteryRect(0, 0, 50, 30);
	for (int i = 0; i < interface->GetBatteryCount(); i++) {
		ExtPowerStatusView* view = new ExtPowerStatusView(interface,
			batteryRect, B_FOLLOW_NONE, i, this);
		view->SetExplicitMaxSize(BSize(70, 80));
		view->SetExplicitMinSize(BSize(70, 80));

		batteryLayout->AddView(view);
		fBatteryViewList.AddItem(view);
		fDriverInterface->StartWatching(view);
		if (!view->IsCritical())
			fSelectedView = view;
	}

	batteryLayout->AddItem(BSpaceLayoutItem::CreateGlue());

	infoLayout->AddView(fBatteryInfoView);

	if (!fSelectedView && fBatteryViewList.CountItems() > 0)
		fSelectedView = fBatteryViewList.ItemAt(0);
	fSelectedView->Select();

	BSize size = mainLayout->PreferredSize();
	ResizeTo(size.width, size.height);
}


ExtendedInfoWindow::~ExtendedInfoWindow()
{
	for (int i = 0; i < fBatteryViewList.CountItems(); i++)
		fDriverInterface->StopWatching(fBatteryViewList.ItemAt(i));

	fDriverInterface->ReleaseReference();
}


BatteryInfoView*
ExtendedInfoWindow::GetExtendedBatteryInfoView()
{
	return fBatteryInfoView;
}


void
ExtendedInfoWindow::BatterySelected(ExtPowerStatusView* view)
{
	if (fSelectedView) {
		fSelectedView->Select(false);
		fSelectedView->Invalidate();
	}

	fSelectedView = view;
}
