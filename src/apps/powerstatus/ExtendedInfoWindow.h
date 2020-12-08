/*
 * Copyright 2009-2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Kacper Kasper, kacperkasper@gmail.com
 */

#ifndef EXTENDED_INFO_WINDOW_H
#define EXTENDED_INFO_WINDOW_H


#include <ObjectList.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>
#include <View.h>
#include <Window.h>

#include "DriverInterface.h"
#include "PowerStatusView.h"


class BatteryInfoView : public BView {
public:
							BatteryInfoView();
							~BatteryInfoView();

	virtual void			Update(battery_info& info,
								acpi_extended_battery_info& extInfo);
	virtual void			AttachedToWindow();

private:
			BString			_GetTextForLine(size_t line);

			battery_info				fBatteryInfo;
			acpi_extended_battery_info	fBatteryExtendedInfo;

			BObjectList<BStringView>	fStringList;
};


class ExtendedInfoWindow;
class BatteryTabView;

class ExtPowerStatusView : public PowerStatusView {
public:
								ExtPowerStatusView(
									PowerStatusDriverInterface* interface,
									BRect frame, int32 resizingMode,
									int batteryID,
									BatteryInfoView* batteryInfoView,
									ExtendedInfoWindow* window);

	virtual void				Select(bool select = true);

	// return true if it battery is in a critical state
	virtual	bool				IsCritical();

protected:
	virtual void				Update(bool force = false, bool notify = true);

private:
			ExtendedInfoWindow*	fExtendedInfoWindow;
			BatteryInfoView*	fBatteryInfoView;
			BatteryTabView*		fBatteryTabView;

			bool				fSelected;
};


class BatteryTab : public BTab {
public:
						BatteryTab(BatteryInfoView* target,
							ExtPowerStatusView* view);
						~BatteryTab();

	virtual	void		Select(BView* owner);

	virtual	void		DrawFocusMark(BView* owner, BRect frame);
	virtual	void		DrawLabel(BView* owner, BRect frame);
private:
	ExtPowerStatusView*	fBatteryView;
};


class BatteryTabView : public BTabView {
public:
					BatteryTabView(const char* name);
					~BatteryTabView();

	virtual	BRect	TabFrame(int32 index) const;
};


class ExtendedInfoWindow : public BWindow
{
public:
		ExtendedInfoWindow(PowerStatusDriverInterface* interface);
		~ExtendedInfoWindow();

	BatteryTabView*				GetBatteryTabView();

private:
	PowerStatusDriverInterface* 		fDriverInterface;
	BObjectList<ExtPowerStatusView>		fBatteryViewList;

	BatteryTabView*						fBatteryTabView;

	ExtPowerStatusView*					fSelectedView;
};


#endif
