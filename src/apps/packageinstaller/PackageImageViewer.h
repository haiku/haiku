/*
 * Copyright (c) 2007, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Łukasz 'Sil2100' Zemczak <sil2100@vexillium.org>
 */
#ifndef PACKAGEIMAGEVIEWER_H
#define PACKAGEIMAGEVIEWER_H

#include <Window.h>
#include <View.h>
#include <Bitmap.h>
#include <DataIO.h>



class ImageView : public BView {
	public:
		ImageView(BPositionIO *image);
		~ImageView();

		void AttachedToWindow();
		void Draw(BRect updateRect);
		void MouseUp(BPoint point);

	private:
		BBitmap *fImage;
		bool fSuccess;
};


class PackageImageViewer : public BWindow {
	public:
		PackageImageViewer(BPositionIO *image);
		~PackageImageViewer();
		
		void Go();

		void MessageReceived(BMessage *msg);
		
	private:
		ImageView *fBackground;

		sem_id fSemaphore;
};


#endif

