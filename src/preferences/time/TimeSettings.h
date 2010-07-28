/*
 * Copyright 2002-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef _TIME_SETTINGS_H
#define _TIME_SETTINGS_H


#include <Point.h>
#include <String.h>


class TimeSettings {
public :
								TimeSettings();
								~TimeSettings();

			BPoint				LeftTop() const;
			void				SetLeftTop(const BPoint leftTop);

private:
			BString				fSettingsFile;
};


#endif	// _TIME_SETTINGS_H

