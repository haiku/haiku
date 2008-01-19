/*
 * Copyright 2007, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CENTERED_VIEW_CONTAINER_H
#define CENTERED_VIEW_CONTAINER_H


#include <View.h>



class CenteredViewContainer : public BView {
public:
	CenteredViewContainer(BView *target, BRect frame, const char* name, uint32 resizingMode);
	virtual ~CenteredViewContainer();

	void Draw(BRect updateRect);
	void FrameResized(float width, float height);

private:
	void _CenterTarget(float width, float height);

	BView *fTarget;
};

#endif // CENTERED_VIEW_CONTAINER_H
