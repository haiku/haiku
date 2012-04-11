/*
 * Copyright 2012 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GEARS_VIEW_H
#define GEARS_VIEW_H


#include <View.h>


class BBitmap;


class GearsView : public BView {
public:
							GearsView();
	virtual					~GearsView();

	virtual	void			Draw(BRect updateRect);

protected:
			void			_InitGearsBitmap();

private:
			BBitmap*		fGears;
};


#endif /* OPENGL_VIEW_H */
