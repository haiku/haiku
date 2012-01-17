/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ALMGROUP_H
#define ALMGROUP_H	


#include <vector>

#include <InterfaceDefs.h> // for enum orientation
#include <Referenceable.h>

#include "Tab.h"


class BLayoutItem;
class BView;


namespace BALM {

class BALMLayout;


class ALMGroup {
public:
								ALMGroup(BLayoutItem* item);
								ALMGroup(BView* view);

			BLayoutItem*		LayoutItem() const;
			BView*				View() const;

			const std::vector<ALMGroup>& Groups() const;
			enum orientation	Orientation() const;

			ALMGroup& 			operator|(const ALMGroup& right);
			ALMGroup& 			operator/(const ALMGroup& bottom);

			void				BuildLayout(BALMLayout* layout,
									XTab* left = NULL, YTab* top = NULL,
									XTab* right = NULL, YTab* bottom = NULL);

private:
								ALMGroup();

			void				_Init(BLayoutItem* item, BView* view,
									  enum orientation orien = B_HORIZONTAL);
			ALMGroup& 			_AddItem(const ALMGroup& item,
									enum orientation orien);

			void				_Build(BALMLayout* layout,
									BReference<XTab> left, BReference<YTab> top,
									BReference<XTab> right,
									BReference<YTab> bottom) const;


			BLayoutItem*		fLayoutItem;
			BView*				fView;

			std::vector<ALMGroup> fGroups;
			enum orientation	fOrientation;

			uint32				_reserved[4];
};


};


using BALM::ALMGroup;

#endif
