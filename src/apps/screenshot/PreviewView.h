/*
 * Copyright 2009, Philippe Saint-Pierre, stpere@gmail.com
 * Distributed under the terms of the MIT License.
 */
#ifndef PREVIEW_VIEW_H
#define PREVIEW_VIEW_H


#include <View.h>


class PreviewView : public BView {
public:
						PreviewView();
						~PreviewView();

protected:
	virtual void		Draw(BRect updateRect);
};

#endif	/* PREVIEW_VIEW_H */
