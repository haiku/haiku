/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */

#include "ExtendedInfoWindow.h"

#include <Box.h>
#include <GroupLayout.h>
#include <GroupView.h>
#include <SpaceLayoutItem.h>
#include <String.h>


BatteryInfoView::BatteryInfoView(BRect frame, int32 resizingMode)
	:
	BView(frame, "battery info view", resizingMode, B_WILL_DRAW |
		B_FULL_UPDATE_ON_RESIZE)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
BatteryInfoView::Update(battery_info& info, acpi_extended_battery_info& extInfo)
{
	fBatteryInfo = info;
	fBatteryExtendedInfo = extInfo;
}


void
BatteryInfoView::Draw(BRect updateRect)
{
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BString powerUnit;
	BString rateUnit;
	switch (fBatteryExtendedInfo.power_unit) {
		case 0:
			powerUnit = " mWh";
			rateUnit = " mW";
			break;
		
		case 1:
			powerUnit = " mAh";
			rateUnit = " mA";
			break;
	}

	BString text;
	if (fBatteryInfo.state & BATTERY_CHARGING)
		text = "Battery charging";
	else if (fBatteryInfo.state & BATTERY_DISCHARGING)
		text = "Battery discharging";
	else if (fBatteryInfo.state & BATTERY_CRITICAL_STATE)
		text = "Empty Battery Slot";
	else
		text = "Battery unused";
	BPoint point(10, 10);
	int textHeight = 15;
	int space = textHeight + 5;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Capacity: ";
	text << fBatteryInfo.capacity;
	text << powerUnit;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Last full Charge: ";
	text << fBatteryInfo.full_capacity;
	text << powerUnit;
	DrawString(text.String(), point);
	point.y += space;

	text = "Current Rate: ";
	text << fBatteryInfo.current_rate;
	text << rateUnit;
	DrawString(text.String(), point);
	point.y += space;
	
	point.y += space;

	text = "Design Capacity: ";
	text << fBatteryExtendedInfo.design_capacity;
	text << powerUnit;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Technology: ";
	text << fBatteryExtendedInfo.technology;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Design Voltage: ";
	text << fBatteryExtendedInfo.design_voltage;
	text << " mV";
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Design Capacity Warning: ";
	text << fBatteryExtendedInfo.design_capacity_warning;
	text << powerUnit;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Design Capacity low Warning: ";
	text << fBatteryExtendedInfo.design_capacity_low;
	text << powerUnit;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Capacity Granularity 1: ";
	text << fBatteryExtendedInfo.capacity_granularity_1;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Capacity Granularity 2: ";
	text << fBatteryExtendedInfo.capacity_granularity_2;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Model Number: ";
	text << fBatteryExtendedInfo.model_number;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Serial number: ";
	text << fBatteryExtendedInfo.serial_number;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "Type: ";
	text += fBatteryExtendedInfo.type;
	DrawString(text.String(), point);
	point.y += space;
	
	text = "OEM Info: ";
	text += fBatteryExtendedInfo.oem_info;
	DrawString(text.String(), point);
	point.y += space;
	
}		
		

ExtPowerStatusView::ExtPowerStatusView(PowerStatusDriverInterface* interface,
			BRect frame, int32 resizingMode, int batteryId,
			ExtendedInfoWindow* window)
	:
	PowerStatusView(interface, frame, resizingMode, batteryId),
	fExtendedInfoWindow(window),
	fBatteryInfoView(window->GetExtendedBatteryInfoView()),
	fSelected(false)
{
	
}


void
ExtPowerStatusView::Draw(BRect updateRect)
{
	if (fSelected) {
		SetLowColor(102, 152, 203);
		SetHighColor(102, 152, 203);
		FillRect(updateRect);	
	}
	else {
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		FillRect(updateRect);
	}

	PowerStatusView::Draw(updateRect);
}


void
ExtPowerStatusView::MouseDown(BPoint where)
{
	if (!fSelected) {
		fSelected = true;
		_Update(true);
		fExtendedInfoWindow->BatterySelected(this);
	}
}		


void
ExtPowerStatusView::Select(bool select)
{
	fSelected = select;
	_Update(true);
}


bool
ExtPowerStatusView::IsValid()
{
	if (fBatteryInfo.state & BATTERY_CRITICAL_STATE)
		return false;
	return true;
}


void
ExtPowerStatusView::_Update(bool force)
{
	PowerStatusView::_Update(force);
	if (!fSelected)
		return;
		
	acpi_extended_battery_info extInfo;
	fDriverInterface->GetExtendedBatteryInfo(&extInfo, fBatteryId);

	fBatteryInfoView->Update(fBatteryInfo, extInfo);
	fBatteryInfoView->Invalidate();
}


ExtendedInfoWindow::ExtendedInfoWindow(PowerStatusDriverInterface* interface)
	:
	BWindow(BRect(100, 150, 500, 500), "Extended Battery Info", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS),
	fDriverInterface(interface),
	fSelectedView(NULL)
{
	BView *view = new BView(Bounds(), "view", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	BGroupLayout* mainLayout = new BGroupLayout(B_VERTICAL);
	mainLayout->SetSpacing(10);
	mainLayout->SetInsets(10, 10, 10, 10);
	view->SetLayout(mainLayout);
	
	BRect rect = Bounds();
	rect.InsetBy(5, 5);
	BBox *infoBox = new BBox(rect, "Power Status Box");
	infoBox->SetLabel("Battery Info");
	BGroupLayout* infoLayout = new BGroupLayout(B_HORIZONTAL);
	infoLayout->SetInsets(10, infoBox->TopBorderOffset() * 2 + 10, 10, 10);
	infoLayout->SetSpacing(10);
	infoBox->SetLayout(infoLayout);
	mainLayout->AddView(infoBox);
	
	BGroupView* batteryView = new BGroupView(B_VERTICAL);
	batteryView->GroupLayout()->SetSpacing(10);
	infoLayout->AddView(batteryView);

	fBatteryInfoView = new BatteryInfoView(BRect(0, 0, 270, 310), B_FOLLOW_ALL);
	
	BGroupLayout* batteryLayout = batteryView->GroupLayout();
	BRect batteryRect(0, 0, 50, 30);
	for (int i = 0; i < interface->GetBatteryCount(); i++) {
		ExtPowerStatusView* view = new ExtPowerStatusView(interface,
			batteryRect, B_FOLLOW_ALL, i, this);
		batteryLayout->AddView(view);
		fBatteryViewList.AddItem(view);
		fDriverInterface->StartWatching(view);
		if (view->IsValid())
			fSelectedView = view;
	}

	batteryLayout->AddItem(BSpaceLayoutItem::CreateGlue());	
	
	infoLayout->AddView(fBatteryInfoView, 20);
	
	if (!fSelectedView && fBatteryViewList.CountItems() > 0)
		fSelectedView = fBatteryViewList.ItemAt(0);	
	fSelectedView->Select();
	
	BSize size = mainLayout->PreferredSize();
	ResizeTo(size.width, size.height);
}


ExtendedInfoWindow::~ExtendedInfoWindow()
{
	for (int i = 0; i < fBatteryViewList.CountItems(); i++) {
		fDriverInterface->StopWatching(fBatteryViewList.ItemAt(i));
	}
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
