/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 */

#ifndef POWER_STATUS_VIEW_H
#define POWER_STATUS_VIEW_H


#include <View.h>

#include "DriverInterface.h"


class PowerStatusView : public BView {
	public:
		PowerStatusView(PowerStatusDriverInterface* interface,
			BRect frame, int32 resizingMode, int batteryId = -1,
			bool inDeskbar = false);
		
		virtual	~PowerStatusView();

		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	AttachedToWindow();
		virtual	void	DetachedFromWindow();

		virtual	void	MessageReceived(BMessage* message);
		virtual	void	Draw(BRect updateRect);
		virtual void	GetPreferredSize(float *width, float *height);
		
	protected:
						PowerStatusView(BMessage* archive);
		
		virtual void	_Update(bool force = false);
		virtual void	_GetBatteryInfo(battery_info* info, int batteryId);

		PowerStatusDriverInterface*	fDriverInterface;

		bool			fShowLabel;
		bool			fShowTime;
		bool			fShowStatusIcon;
		
		int				fBatteryId;
		bool			fInDeskbar;

		battery_info	fBatteryInfo;

	private:
		void			_Init();
		void			_SetLabel(char* buffer, size_t bufferLength);
		void			_DrawBattery(BRect rect);

		int32			fPercent;
		time_t			fTimeLeft;
		bool			fOnline;

		BSize			fPreferredSize;
};


class PowerStatusReplicant : public PowerStatusView
{
	public:
		PowerStatusReplicant(BRect frame, int32 resizingMode,
			bool inDeskbar = false);
		PowerStatusReplicant(BMessage* archive);

		virtual	~PowerStatusReplicant();

		static	PowerStatusReplicant* Instantiate(BMessage* archive);
		virtual	status_t Archive(BMessage* archive, bool deep = true) const;

		virtual	void	MessageReceived(BMessage* message);
		virtual	void	MouseDown(BPoint where);

	private:
		void			_AboutRequested();
		void			_Init();
		void			_Quit();
		
		void			_OpenExtendedWindow();

		BWindow*		fExtendedWindow;
		bool			fMessengerExist;
		BMessenger*		fExtWindowMessenger;		
};


#endif	// POWER_STATUS_VIEW_H
