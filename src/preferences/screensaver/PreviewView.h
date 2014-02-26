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


class BTextView;

class PreviewView : public BView {
public:
								PreviewView(const char* name);
	virtual						~PreviewView();

	virtual	void				Draw(BRect updateRect);

			BView*				AddPreview();
			BView*				RemovePreview();
			BView*				SaverView();

			void				ShowNoPreview() const;
			void				HideNoPreview() const;

private:
			BView*				fSaverView;
			BTextView*			fNoPreview;
};


#endif	// PREVIEW_VIEW_H
