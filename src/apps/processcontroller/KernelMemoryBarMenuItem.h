/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_MEMORY_BAR_MENU_ITEM_H_
#define _KERNEL_MEMORY_BAR_MENU_ITEM_H_


#include <MenuItem.h>


class KernelMemoryBarMenuItem : public BMenuItem {
	public:
		KernelMemoryBarMenuItem(system_info& systemInfo);
		virtual	void	DrawContent();
		virtual	void	GetContentSize(float* _width, float* _height);

		void			DrawBar(bool force);
		void			UpdateSituation(int64 committedMemory, int64 fCachedMemory);

	private:
		int64	fCachedMemory;
		int64	fPhysicalMemory;
		int64	fCommittedMemory;
		double	fLastSum;
		double	fGrenze1;
		double	fGrenze2;
};

#endif // _KERNEL_MEMORY_BAR_MENU_ITEM_H_
