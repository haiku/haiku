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
	XTab(LinearSpec* ls)
		:
		Variable(ls)
	{
		
	}

public:
	friend	class				BALMLayout;
};


class YTab : public Variable {
protected:
	YTab(LinearSpec* ls)
		:
		Variable(ls)
	{
		
	}

public:
	friend	class				BALMLayout;
};


}	// namespace BALM


using BALM::XTab;
using BALM::YTab;

typedef BObjectList<XTab> XTabList;
typedef BObjectList<YTab> YTabList;


#endif	// X_TAB_H
