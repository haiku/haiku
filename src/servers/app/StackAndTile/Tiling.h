/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef TILING_H
#define TILING_H

#include "ObjectList.h"

#include "Decorator.h"
#include "StackAndTile.h"
#include "SATGroup.h"


class SATWindow;


class SATTiling : public SATSnappingBehaviour {
public:
							SATTiling(SATWindow* window);
							~SATTiling();

		bool				FindSnappingCandidates(SATGroup* group);
		bool				JoinCandidates();

		void				WindowLookChanged(window_look look);
private:
		bool				_IsTileableWindow(Window* window);

		bool				_FindFreeAreaInGroup(SATGroup* group);
		bool				_FindFreeAreaInGroup(SATGroup* group,
								Corner::position_t corner);

		bool				_InteresstingCrossing(Crossing* crossing,
								Corner::position_t corner, BRect& windowFrame);
		bool				_FindFreeArea(SATGroup* group,
								const Crossing* crossing,
								Corner::position_t areaCorner,
								BRect& windowFrame);
		bool				_HasOverlapp(SATGroup* group);
		bool				_CheckArea(SATGroup* group,
								Corner::position_t corner, BRect& windowFrame,
								float& error);
		bool				_CheckMinFreeAreaSize();
		float				_FreeAreaError(BRect& windowFrame);
		bool				_IsCornerInFreeArea(Corner::position_t corner,
								BRect& windowFrame);

		BRect				_FreeAreaSize();

		void				_HighlightWindows(SATGroup* group,
								bool highlight = true);
		bool				_SearchHighlightWindow(Tab* tab, Tab* firstOrthTab,
								Tab* secondOrthTab, const TabList* orthTabs,
								Corner::position_t areaCorner,
								Decorator::Region region, bool highlight);
		void				_HighlightWindows(WindowArea* area,
								Decorator::Region region, bool highlight);

		void				_ResetSearchResults();

		SATWindow*			fSATWindow;

		SATGroup*			fFreeAreaGroup;
		Tab*				fFreeAreaLeft;
		Tab*				fFreeAreaRight;
		Tab*				fFreeAreaTop;
		Tab*				fFreeAreaBottom;
};

#endif
