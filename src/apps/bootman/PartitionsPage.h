/*
 * Copyright 2008, Michael Pfeiffer, laplace@users.sourceforge.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PARTITONS_PAGE_H
#define PARTITONS_PAGE_H


#include "WizardPageView.h"


class BTextView;
class BScrollView;

class PartitionsPage : public WizardPageView
{
public:
	PartitionsPage(BMessage* settings, BRect frame, const char* name);
	virtual ~PartitionsPage();
	
	virtual void PageCompleted();
	
	virtual void FrameResized(float width, float height);

private:

	void _BuildUI();
	void _Layout();
	void _FillPartitionsView(BView* view);
	void _CreateSizeText(int64 size, BString* text);
	BMessage* _CreateControlMessage(uint32 what, int32 partitionIndex);
	void _ComputeColumnWidths(int32& showWidth, int32& nameWidth, int32& typeWidth, 
		int32& sizeWidth, int32& pathWidth);
	
	BTextView* fDescription;
	BView* fPartitions;
	BScrollView* fPartitionsScrollView;
	float fPartitionsWidth;
	float fPartitionsHeight;
};

#endif	// PARTITONS_PAGE_H
