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


class TabBase {
private:
								TabBase();
	virtual						~TabBase();

			friend class BALMLayout;
			friend class XTab;
			friend class YTab;
			struct BALMLayoutList;

			bool				IsInLayout(BALMLayout* layout);
			bool				AddedToLayout(BALMLayout* layout);
			void				LayoutLeaving(BALMLayout* layout);
			bool				IsSuitableFor(BALMLayout* layout);

			BALMLayoutList*		fLayouts;
};


/**
 * Vertical grid line (x-tab).
 */
class XTab : public Variable, public TabBase, public BReferenceable {
public:
	virtual						~XTab();

protected:
	friend	class				BALMLayout;
								XTab(BALMLayout* layout);

private:
			uint32				_reserved[2];
};


class YTab : public Variable, public TabBase, public BReferenceable {
public:
	virtual						~YTab();

protected:
	friend	class				BALMLayout;
								YTab(BALMLayout* layout);
private:
			uint32				_reserved[2];
};


}	// namespace BALM


using BALM::XTab;
using BALM::YTab;

typedef BObjectList<XTab> XTabList;
typedef BObjectList<YTab> YTabList;


#endif	// X_TAB_H
