/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGE_IMAGE_VIEWER_H
#define PACKAGE_IMAGE_VIEWER_H

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <DataIO.h>



class ImageView : public BView {
public:
								ImageView(BPositionIO* image);
	virtual						~ImageView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseUp(BPoint point);

private:
			BBitmap*			fImage;
};


class PackageImageViewer : public BWindow {
public:
								PackageImageViewer(BPositionIO* image);
	virtual						~PackageImageViewer();
		
	virtual	bool				QuitRequested();
		
			void				Go();

private:
			ImageView*			fBackground;

			sem_id				fSemaphore;
};


#endif // PACKAGE_IMAGE_VIEWER_H

