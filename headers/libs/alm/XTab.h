/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	X_TAB_H
#define	X_TAB_H

#include "LinearSpec.h"
#include "Variable.h"


namespace BALM {


/**
 * Vertical grid line (x-tab).
 */
class XTab : public Variable {
protected:
								XTab(LinearSpec* ls);

protected:
			/**
			* Property signifying if there is a constraint which relates
			* this tab to a different tab that is further to the left.
			* Only used for reverse engineering.
			*/
			bool				fLeftLink;

public:
	friend class			Area;
	friend class			Column;
	friend class			BALMLayout;

};

}	// namespace BALM

using BALM::XTab;

#endif	// X_TAB_H
