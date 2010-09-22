/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	Y_TAB_H
#define	Y_TAB_H


#include "LinearSpec.h"
#include "Variable.h"


namespace BALM {


/**
 * Horizontal grid line (y-tab).
 */
class YTab : public Variable {
protected:
								YTab(LinearSpec* ls);

protected:
			/**
			* Property signifying if there is a constraint which relates
			* this tab to a different tab that is further to the top.
			* Only used for reverse engineering.
			*/
			bool				fTopLink;

public:
	friend class			Area;
	friend class			Row;
	friend class			BALMLayout;

};

}	// namespace BALM

using BALM::YTab;

#endif	// Y_TAB_H
