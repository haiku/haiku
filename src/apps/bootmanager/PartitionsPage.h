/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef PARTITONS_PAGE_H
#define PARTITONS_PAGE_H


#include "WizardPageView.h"


class BGridView;
class BTextView;
class BScrollView;


class PartitionsPage : public WizardPageView {
public:
								PartitionsPage(BMessage* settings,
									const char* name);
	virtual						~PartitionsPage();

	virtual	void				PageCompleted();

private:
			void				_BuildUI();
			void				_FillPartitionsView(BView* view);
			void				_CreateSizeText(int64 size, BString* text);
			BMessage*			_CreateControlMessage(uint32 what,
									int32 partitionIndex);

private:
			BTextView*			fDescription;
			BGridView*			fPartitions;
};


#endif	// PARTITONS_PAGE_H
