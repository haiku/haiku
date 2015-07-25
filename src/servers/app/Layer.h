/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef LAYER_H
#define LAYER_H


#include "ServerPicture.h"

#include "IntPoint.h"


class AlphaMask;
class Canvas;
class UtilityBitmap;


class Layer : public ServerPicture {
public:
								Layer(uint8 opacity);
	virtual						~Layer();

			void				PushLayer(Layer* layer);
			Layer*				PopLayer();

			UtilityBitmap*		RenderToBitmap(Canvas* canvas);

			IntPoint			LeftTopOffset() const;
			uint8				Opacity() const;

private:
			BRect				_DetermineBoundingBox(Canvas* canvas);
			UtilityBitmap*		_AllocateBitmap(const BRect& bounds);

private:
			uint8				fOpacity;
			IntPoint			fLeftTopOffset;
};


#endif // LAYER_H
