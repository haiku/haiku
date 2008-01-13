/*

	NoiseBarMenuItem.h

	ProcessController
	© 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	

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
