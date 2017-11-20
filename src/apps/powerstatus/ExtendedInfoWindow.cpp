/*
 * Copyright 2009-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Kacper Kasper, kacperkasper@gmail.com
 */


#include "ExtendedInfoWindow.h"

#include <Box.h>
#include <Catalog.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PowerStatus"


const size_t kLinesCount = 16;


//	#pragma mark -


BatteryInfoView::BatteryInfoView()
	:
	BView("battery info view", B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	SetLayout(new BGroupLayout(B_VERTICAL, B_USE_ITEM_SPACING));

	for (size_t i = 0; i < kLinesCount; i++) {
		BStringView* view = new BStringView("info", "");
		AddChild(view);
		fStringList.AddItem(view);
	}
	fStringList.ItemAt(0)->SetFont(be_bold_font);
	AddChild(BSpaceLayoutItem::CreateGlue());
}


BatteryInfoView::~BatteryInfoView()
{
	for (int32 i = 0; i < fStringList.CountItems(); i++)
		delete fStringList.ItemAt(i);
}


void
BatteryInfoView::Update(battery_info& info, acpi_extended_battery_info& extInfo)
{
	fBatteryInfo = info;
	fBatteryExtendedInfo = extInfo;

	for (size_t i = 0; i < kLinesCount; i++) {
		fStringList.ItemAt(i)->SetText(_GetTextForLine(i));
	}
}


void
BatteryInfoView::AttachedToWindow()
{
	Window()->CenterOnScreen();
}


BString
BatteryInfoView::_GetTextForLine(size_t line)
{
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

	BString string;
	switch (line) {
		case 0: {
			if ((fBatteryInfo.state & BATTERY_CHARGING) != 0)
				string = B_TRANSLATE("Battery charging");
			else if ((fBatteryInfo.state & BATTERY_DISCHARGING) != 0)
				string = B_TRANSLATE("Battery discharging");
			else if ((fBatteryInfo.state & BATTERY_CRITICAL_STATE) != 0
				&& fBatteryExtendedInfo.model_number[0] == '\0'
				&& fBatteryExtendedInfo.serial_number[0] == '\0'
				&& fBatteryExtendedInfo.type[0] == '\0'
				&& fBatteryExtendedInfo.oem_info[0] == '\0')
				string = B_TRANSLATE("Empty battery slot");
			else if ((fBatteryInfo.state & BATTERY_CRITICAL_STATE) != 0)
				string = B_TRANSLATE("Damaged battery");
			else
				string = B_TRANSLATE("Battery unused");
			break;
		}
		case 1:
			string = B_TRANSLATE("Capacity: ");
			string << fBatteryInfo.capacity;
			string << powerUnit;
			break;
		case 2:
			string = B_TRANSLATE("Last full charge: ");
			string << fBatteryInfo.full_capacity;
			string << powerUnit;
			break;
		case 3:
			string = B_TRANSLATE("Current rate: ");
			string << fBatteryInfo.current_rate;
			string << rateUnit;
			break;
		// case 4 missed intentionally
		case 5:
			string = B_TRANSLATE("Design capacity: ");
			string << fBatteryExtendedInfo.design_capacity;
			string << powerUnit;
			break;
		case 6:
			string = B_TRANSLATE("Technology: ");
			if (fBatteryExtendedInfo.technology == 0)
				string << B_TRANSLATE("non-rechargeable");
			else if (fBatteryExtendedInfo.technology == 1)
				string << B_TRANSLATE("rechargeable");
			else
				string << "?";
			break;
		case 7:
			string = B_TRANSLATE("Design voltage: ");
			string << fBatteryExtendedInfo.design_voltage;
			string << B_TRANSLATE(" mV");
			break;
		case 8:
			string = B_TRANSLATE("Design capacity warning: ");
			string << fBatteryExtendedInfo.design_capacity_warning;
			string << powerUnit;
			break;
		case 9:
			string = B_TRANSLATE("Design capacity low warning: ");
			string << fBatteryExtendedInfo.design_capacity_low;
			string << powerUnit;
			break;
		case 10:
			string = B_TRANSLATE("Capacity granularity 1: ");
			string << fBatteryExtendedInfo.capacity_granularity_1;
			string << powerUnit;
			break;
		case 11:
			string = B_TRANSLATE("Capacity granularity 2: ");
			string << fBatteryExtendedInfo.capacity_granularity_2;
			string << powerUnit;
			break;
		case 12:
			string = B_TRANSLATE("Model number: ");
			string << fBatteryExtendedInfo.model_number;
			break;
		case 13:
			string = B_TRANSLATE("Serial number: ");
			string << fBatteryExtendedInfo.serial_number;
			break;
		case 14:
			string = B_TRANSLATE("Type: ");
			string += fBatteryExtendedInfo.type;
			break;
		case 15:
			string = B_TRANSLATE("OEM info: ");
			string += fBatteryExtendedInfo.oem_info;
			break;
		default:
			string = "";
			break;
	}
	return string;
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
	if (fSelected) {
		rgb_color lowColor = LowColor();
		SetLowColor(102, 152, 203);
		FillRect(updateRect, B_SOLID_LOW);
		SetLowColor(lowColor);
	}
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
	fDriverInterface->GetExtendedBatteryInfo(fBatteryID, &extInfo);

	fBatteryInfoView->Update(fBatteryInfo, extInfo);
	fBatteryInfoView->Invalidate();
}


//	#pragma mark -


ExtendedInfoWindow::ExtendedInfoWindow(PowerStatusDriverInterface* interface)
	:
	BWindow(BRect(100, 150, 500, 500), B_TRANSLATE("Extended battery info"),
		B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AVOID_FRONT
			| B_ASYNCHRONOUS_CONTROLS),
	fDriverInterface(interface),
	fSelectedView(NULL)
{
	fDriverInterface->AcquireReference();

	BView *view = new BView(Bounds(), "view", B_FOLLOW_ALL, 0);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
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
