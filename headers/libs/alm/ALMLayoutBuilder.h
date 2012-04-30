/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	ALM_LAYOUT_BUILDER_H
#define	ALM_LAYOUT_BUILDER_H


#include "ALMLayout.h"

#include <ObjectList.h>


class BLayoutItem;
class BView;
class BWindow;


namespace BALM {


class BALMLayoutBuilder {
public:
								BALMLayoutBuilder(BView* view, float hSpacing,
									float vSpacing,
									BALMLayout* friendLayout = NULL);
								BALMLayoutBuilder(BView* view,
									BALMLayout* layout);
								BALMLayoutBuilder(BWindow* window,
									float hSpacing, float vSpacing,
									BALMLayout* friendLayout = NULL);
								BALMLayoutBuilder(BWindow* window,
									BALMLayout* layout);
								BALMLayoutBuilder(BALMLayout* layout);

	BALMLayoutBuilder&			Add(BView* view, XTab* left, YTab* top,
									XTab* right = NULL, YTab* bottom = NULL);
	BALMLayoutBuilder&			Add(BView* view, Row* row, Column* column);

	BALMLayoutBuilder&			Add(BLayoutItem* item, XTab* left,
									YTab* top, XTab* right = NULL,
									YTab* bottom = NULL);
	BALMLayoutBuilder&			Add(BLayoutItem* item, Row* row,
									Column* column);

	BALMLayoutBuilder&			SetInsets(float insets);
	BALMLayoutBuilder&			SetInsets(float horizontal, float vertical);
	BALMLayoutBuilder&			SetInsets(float left, float top, float right,
									float bottom);

	BALMLayoutBuilder&			SetSpacing(float horizontal, float vertical);
									
	BALMLayoutBuilder&			AddToLeft(BView* view,
									XTab* left = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	BALMLayoutBuilder&			AddToRight(BView* view,
									XTab* right = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	BALMLayoutBuilder&			AddAbove(BView* view,
									YTab* top = NULL, XTab* left = NULL,
									XTab* right = NULL);
	BALMLayoutBuilder&			AddBelow(BView* view, YTab* bottom = NULL,
									XTab* left = NULL, XTab* right = NULL);

	BALMLayoutBuilder&			AddToLeft(BLayoutItem* item,
									XTab* left = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	BALMLayoutBuilder&			AddToRight(BLayoutItem* item,
									XTab* right = NULL, YTab* top = NULL,
									YTab* bottom = NULL);
	BALMLayoutBuilder&			AddAbove(BLayoutItem* item,
									YTab* top = NULL, XTab* left = NULL,
									XTab* right = NULL);
	BALMLayoutBuilder&			AddBelow(BLayoutItem* item,
									YTab* bottom = NULL, XTab* left = NULL,
									XTab* right = NULL);


		BALMLayoutBuilder&			Push();
		BALMLayoutBuilder&			Pop();


		// these methods throw away the stack
		BALMLayoutBuilder&			StartingAt(BView* view);
		BALMLayoutBuilder&			StartingAt(BLayoutItem* item);


private:
	typedef BObjectList<Area>	AreaStack;

		BALMLayout*				fLayout;
		AreaStack				fAreaStack;

		Area*					_CurrentArea() const;
		void					_SetCurrentArea(Area*);
};


};


#endif
