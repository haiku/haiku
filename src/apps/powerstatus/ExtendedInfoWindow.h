/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */
 
#ifndef EXTENDED_INFO_WINDOW_H
#define EXTENDED_INFO_WINDOW_H

#include <ObjectList.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#include "DriverInterface.h"
#include "PowerStatusView.h"


class BatteryInfoView : public BView
{
	public:
						BatteryInfoView(BRect frame, int32 resizingMode);

		virtual void	Update(battery_info& info,
							acpi_extended_battery_info& extInfo);
		virtual	void	Draw(BRect updateRect);
	
	private:
		battery_info	fBatteryInfo;
		acpi_extended_battery_info fBatteryExtendedInfo;
};


class ExtendedInfoWindow;

class ExtPowerStatusView : public PowerStatusView
{
	public:
		ExtPowerStatusView(PowerStatusDriverInterface* interface,
			BRect frame, int32 resizingMode, int batteryId,
			ExtendedInfoWindow* window);
			
		virtual	void	Draw(BRect updateRect);
		virtual	void	MouseDown(BPoint where);
	
		virtual void	Select(bool select = true);
		
		// return true if it battery is in a none critical state
		virtual bool	IsValid();

	protected:
		virtual void	_Update(bool force = false);

	private:
		ExtendedInfoWindow*	fExtendedInfoWindow;
		BatteryInfoView*	fBatteryInfoView;
		
		bool				fSelected;	
};


class ExtendedInfoWindow : public BWindow
{
public:
		ExtendedInfoWindow(PowerStatusDriverInterface* interface);
		~ExtendedInfoWindow();
	
	BatteryInfoView*			GetExtendedBatteryInfoView();

	void						BatterySelected(ExtPowerStatusView* view);

private:
	PowerStatusDriverInterface* 		fDriverInterface;
	BObjectList<ExtPowerStatusView>		fBatteryViewList;
	
	BatteryInfoView*					fBatteryInfoView;
	
	ExtPowerStatusView*					fSelectedView;
};


#endif
