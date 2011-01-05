/*
 * Copyright 2008-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */
#ifndef DESCRIPTION_PAGE_H
#define DESCRIPTION_PAGE_H


#include "WizardPageView.h"


class BTextView;

class DescriptionPage : public WizardPageView {
public:
								DescriptionPage(const char* name,
									const char* description, bool hasHeading);
	virtual						~DescriptionPage();

private:
			void				_BuildUI(const char* description,
									bool hasHeading);

private:
			BTextView*			fDescription;
};


#endif	// DESCRIPTION_PAGE_H
