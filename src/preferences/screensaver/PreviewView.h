/*
 * Copyright 2003-2013 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#ifndef PREVIEW_VIEW_H
#define PREVIEW_VIEW_H


#include <Box.h>
#include <ScreenSaverRunner.h>
#include <View.h>


class PreviewView : public BView {
public:
								PreviewView(const char* name);
	virtual						~PreviewView();

	virtual	void				Draw(BRect updateRect);

			BView*				AddPreview();
			BView*				RemovePreview();
			BView*				SaverView();

private:
			BView*				fSaverView;
};


#endif	// PREVIEW_VIEW_H
