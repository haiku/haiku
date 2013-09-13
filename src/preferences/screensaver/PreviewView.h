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


#include <View.h>
#include <Box.h>
#include <ScreenSaverRunner.h>


class PreviewView : public BView { 
public:
								PreviewView(const char* name);
	virtual						~PreviewView();

	virtual	void				Draw(BRect update);

			BView*				AddPreview();
			BView*				RemovePreview();

private:
			BView*				fSaverView;
};


#endif	// PREVIEW_VIEW_H
