/*
 * Copyright 2009-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Kacper Kasper, kacperkasper@gmail.com
 */


#include "ExtendedInfoWindow.h"

#include <ControlLook.h>
#include <Catalog.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <TabView.h>


#include <algorithm>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PowerStatus"


const size_t kLinesCount = 16;


//	#pragma mark -


BatteryInfoView::BatteryInfoView()
	:
	BView("battery info view", B_AUTO_UPDATE_SIZE_LIMITS)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	layout->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		0, B_USE_DEFAULT_SPACING);
	SetLayout(layout);

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


BatteryTab::BatteryTab(BatteryInfoView* target,
		ExtPowerStatusView* view)
	:
	fBatteryView(view)
{
}


BatteryTab::~BatteryTab()
{
}


void
BatteryTab::Select(BView* owner)
{
	BTab::Select(owner);
	fBatteryView->Select();
}

void
BatteryTab::DrawFocusMark(BView* owner, BRect frame)
{
	float vertOffset = IsSelected() ? 3 : 2;
	float horzOffset = IsSelected() ? 2 : 4;
	float width = frame.Width() - horzOffset * 2;
	BPoint pt1((frame.left + frame.right - width) / 2.0 + horzOffset,
		frame.bottom - vertOffset);
	BPoint pt2((frame.left + frame.right + width) / 2.0,
		frame.bottom - vertOffset);
	owner->SetHighUIColor(B_KEYBOARD_NAVIGATION_COLOR);
	owner->StrokeLine(pt1, pt2);
}


void
BatteryTab::DrawLabel(BView* owner, BRect frame)
{
	BRect rect = frame;
	float size = std::min(rect.Width(), rect.Height());
	rect.right = rect.left + size;
	rect.bottom = rect.top + size;
	if (frame.Width() > rect.Height()) {
		rect.OffsetBy((frame.Width() - size) / 2.0f, 0.0f);
	} else {
		rect.OffsetBy(0.0f, (frame.Height() - size) / 2.0f);
	}
	fBatteryView->DrawTo(owner, rect);
}


BatteryTabView::BatteryTabView(const char* name)
	:
	BTabView(name)
{
}


BatteryTabView::~BatteryTabView()
{
}


BRect
BatteryTabView::TabFrame(int32 index) const
{
	BRect bounds(Bounds());
	float width = TabHeight();
	float height = TabHeight();
	float offset = BControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
	switch (TabSide()) {
		case kTopSide:
			return BRect(offset + index * width, 0.0f,
				offset + index * width + width, height);
		case kBottomSide:
			return BRect(offset + index * width, bounds.bottom - height,
				offset + index * width + width, bounds.bottom);
		case kLeftSide:
			return BRect(0.0f, offset + index * width, height,
				offset + index * width + width);
		case kRightSide:
			return BRect(bounds.right - height, offset + index * width,
				bounds.right, offset + index * width + width);
		default:
			return BRect();
	}
}


ExtPowerStatusView::ExtPowerStatusView(PowerStatusDriverInterface* interface,
		BRect frame, int32 resizingMode, int batteryID,
		BatteryInfoView* batteryInfoView, ExtendedInfoWindow* window)
	:
	PowerStatusView(interface, frame, resizingMode, batteryID),
	fExtendedInfoWindow(window),
	fBatteryInfoView(batteryInfoView),
	fBatteryTabView(window->GetBatteryTabView())
{
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
ExtPowerStatusView::Update(bool force, bool notify)
{
	PowerStatusView::Update(force);
	if (!fSelected)
		return;

	acpi_extended_battery_info extInfo;
	fDriverInterface->GetExtendedBatteryInfo(fBatteryID, &extInfo);

	fBatteryInfoView->Update(fBatteryInfo, extInfo);
	fBatteryInfoView->Invalidate();

	fBatteryTabView->Invalidate();
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

	float scale = be_plain_font->Size() / 12.0f;
	float tabHeight = 70.0f * scale;
	BRect batteryRect(0, 0, 50 * scale, 50 * scale);
	fBatteryTabView = new BatteryTabView("tabview");
	fBatteryTabView->SetBorder(B_NO_BORDER);
	fBatteryTabView->SetTabHeight(tabHeight);
	fBatteryTabView->SetTabSide(BTabView::kLeftSide);
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, 0)
		.Add(fBatteryTabView);

	for (int i = 0; i < interface->GetBatteryCount(); i++) {
		BatteryInfoView* batteryInfoView = new BatteryInfoView();
		ExtPowerStatusView* view = new ExtPowerStatusView(interface,
			batteryRect, B_FOLLOW_NONE, i, batteryInfoView, this);
		BatteryTab* tab = new BatteryTab(batteryInfoView, view);
		fBatteryTabView->AddTab(batteryInfoView, tab);
		// Has to be added, otherwise it won't get info updates
		view->Hide();
		AddChild(view);

		fBatteryViewList.AddItem(view);
		fDriverInterface->StartWatching(view);
		if (!view->IsCritical())
			fSelectedView = view;
	}

	if (!fSelectedView && fBatteryViewList.CountItems() > 0)
		fSelectedView = fBatteryViewList.ItemAt(0);
	fSelectedView->Select();

	BSize size = GetLayout()->PreferredSize();
	ResizeTo(size.width, size.height);
}


ExtendedInfoWindow::~ExtendedInfoWindow()
{
	for (int i = 0; i < fBatteryViewList.CountItems(); i++)
		fDriverInterface->StopWatching(fBatteryViewList.ItemAt(i));

	fDriverInterface->ReleaseReference();
}


BatteryTabView*
ExtendedInfoWindow::GetBatteryTabView()
{
	return fBatteryTabView;
}
