/*
 * Copyright 2006 - 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	X_TAB_H
#define	X_TAB_H


#include <Referenceable.h>

#include "Variable.h"


namespace BALM {


class BALMLayout;


/**
 * Vertical grid line (x-tab).
 */
class XTab : public Variable, public BReferenceable {
public:
	virtual						~XTab();

protected:
	friend	class				BALMLayout;
								XTab(BALMLayout* layout);

private:
			BALMLayout*			fALMLayout;

			uint32				_reserved[2];
};


class YTab : public Variable, public BReferenceable {
public:
	virtual						~YTab();

protected:
	friend	class				BALMLayout;
								YTab(BALMLayout* layout);

private:
			BALMLayout*			fALMLayout;

			uint32				_reserved[2];
};


}	// namespace BALM


using BALM::XTab;
using BALM::YTab;

typedef BObjectList<XTab> XTabList;
typedef BObjectList<YTab> YTabList;


#endif	// X_TAB_H
