/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
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


class BFile;


class PowerStatusView : public BView {
public:
							PowerStatusView(
								PowerStatusDriverInterface* interface,
								BRect frame, int32 resizingMode,
								int batteryID = -1, bool inDeskbar = false);

	virtual					~PowerStatusView();

	virtual	status_t		Archive(BMessage* archive, bool deep = true) const;

	virtual	void			AttachedToWindow();
	virtual	void			DetachedFromWindow();

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			Draw(BRect updateRect);
	virtual	void			GetPreferredSize(float *width, float *height);


protected:
							PowerStatusView(BMessage* archive);

	virtual void			Update(bool force = false);

			void			FromMessage(const BMessage* message);
			status_t		ToMessage(BMessage* message) const;

private:
			void			_GetBatteryInfo(battery_info* info, int batteryID);
			void			_Init();
			void			_SetLabel(char* buffer, size_t bufferLength);
			void			_DrawBattery(BRect rect);

protected:
			PowerStatusDriverInterface*	fDriverInterface;

			bool			fShowLabel;
			bool			fShowTime;
			bool			fShowStatusIcon;

			int				fBatteryID;
			bool			fInDeskbar;

			battery_info	fBatteryInfo;

			int32			fPercent;
			time_t			fTimeLeft;
			bool			fOnline;
			bool			fHasBattery;

			BSize			fPreferredSize;
};


class PowerStatusReplicant : public PowerStatusView {
public:
							PowerStatusReplicant(BRect frame,
								int32 resizingMode, bool inDeskbar = false);
							PowerStatusReplicant(BMessage* archive);
	virtual					~PowerStatusReplicant();

	static	PowerStatusReplicant* Instantiate(BMessage* archive);
	virtual	status_t		Archive(BMessage* archive, bool deep = true) const;

	virtual	void			MessageReceived(BMessage* message);
	virtual	void			MouseDown(BPoint where);

private:
			void			_AboutRequested();
			void			_Init();
			void			_Quit();

			status_t		_GetSettings(BFile& file, int mode);
			void			_LoadSettings();
			void			_SaveSettings();

			void			_OpenExtendedWindow();

private:
			BWindow*		fExtendedWindow;
			bool			fMessengerExist;
			BMessenger*		fExtWindowMessenger;
};


#endif	// POWER_STATUS_VIEW_H
