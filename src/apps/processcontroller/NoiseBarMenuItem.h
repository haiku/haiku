/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _NOISE_BAR_MENU_ITEM_H_
#define _NOISE_BAR_MENU_ITEM_H_

#include <MenuItem.h>


class NoiseBarMenuItem : public BMenuItem {
	public:
		NoiseBarMenuItem();

		virtual	void DrawContent();
		virtual	void GetContentSize(float* width, float* height);

		void DrawBar(bool force);

		double BusyWaiting() const
		{
			return fBusyWaiting;
		}

		void SetBusyWaiting(double busyWaiting)
		{
			fBusyWaiting = busyWaiting;
		}

		void SetLost(double lost)
		{
			fLost = lost;
		}

	private:
		double	fBusyWaiting;
		double	fLost;
		float	fGrenze1;
		float	fGrenze2;
};

#endif // _NOISE_BAR_MENU_ITEM_H_
