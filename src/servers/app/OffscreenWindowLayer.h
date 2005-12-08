/*
 * Copyright 2005, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef OFFSCREEN_WINBORDER_H
#define OFFSCREEN_WINBORDER_H


#include "WindowLayer.h"


class BitmapHWInterface;
class ServerBitmap;

class OffscreenWindowLayer : public WindowLayer {
 public:
								OffscreenWindowLayer(ServerBitmap* bitmap,
												   const char* name,
												   ::ServerWindow* window);
	virtual						~OffscreenWindowLayer();
	
	virtual	void				Draw(const BRect &r);
	
	virtual	void				MoveBy(float x, float y);
	virtual	void				ResizeBy(float x, float y);

	virtual	bool				IsOffscreenWindow() const
									{ return true; }

 private:
		ServerBitmap*			fBitmap;
		BitmapHWInterface*		fHWInterface;
};

#endif	// OFFSCREEN_WINBORDER_H
