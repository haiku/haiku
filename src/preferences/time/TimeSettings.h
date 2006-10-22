/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		Andrew McCall, mccall@digitalparadise.co.uk
 *		Mike Berg
 */
#ifndef TIME_SETTINGS_H
#define TIME_SETTINGS_H


#include <Point.h>


class TimeSettings {
	public :
		TimeSettings();
		~TimeSettings();

		BPoint WindowCorner() const { return fCorner; }
		void SetWindowCorner(BPoint corner);

	private:
		static const char kTimeSettingsFile[];
		BPoint fCorner;
};

#endif	// TIME_SETTINGS_H
