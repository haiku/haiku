/*
 * Copyright 2003-2006, Haiku.
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

class BScreenSaver;


class PreviewView : public BView { 
	public:
		PreviewView(BRect frame, const char *name);
		virtual ~PreviewView();

		virtual void Draw(BRect update); 

		BView* AddPreview();
		BView* RemovePreview();

	private:
		BView* fSaverView;
};

#endif	// PREVIEW_VIEW_H
