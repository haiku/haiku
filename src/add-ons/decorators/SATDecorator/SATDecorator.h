/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_DECORATOR_H
#define SAT_DECORATOR_H


#include "DecorManager.h"
#include "DefaultDecorator.h"
#include "StackAndTile.h"


class SATDecorAddOn : public DecorAddOn {
public:
								SATDecorAddOn(image_id id, const char* name);

	virtual status_t			InitCheck() const;
		float					Version();

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);

		StackAndTile			fStackAndTile;
};


class SATDecorator : public DefaultDecorator {
public:
						SATDecorator(DesktopSettings& settings,
							BRect frame, window_look look, uint32 flags);

		void 			HighlightTab(bool active, BRegion* dirty);
		void 			HighlightBorders(bool active, BRegion* dirty);
		bool			IsTabHighlighted() { return fTabHighlighted; }
		bool			IsBordersHighlighted() { return fBordersHighlighted; }

		/*! Indicates that window is stacked */
		void			SetStackedMode(bool stacked, BRegion* dirty);
		bool			StackedMode() { return fStackedMode; };

		/*! Set the tab length if the decorator is in stacked mode and if the 
		tab is the last one in the tab bar. */
		void			SetStackedTabLength(float length, BRegion* dirty);
		float			StackedTabLength() { return fStackedTabLength; }

protected:
		void			_DoLayout();
		void			_DrawTab(BRect r);
		void			_LayoutTabItems(const BRect& tabRect);

		bool			_SetTabLocation(float location,
							BRegion* updateRegion = NULL);
		void			_SetFocus();

private:
		bool			fTabHighlighted;
		bool			fBordersHighlighted;
		rgb_color		fHighlightTabColor;
		rgb_color		fNonHighlightFrameColors[4];
		rgb_color		fHighlightFrameColors[6];

		bool			fStackedMode;
		bool			fStackedDrawZoom;
		float			fStackedTabLength;
		bool			fStackedTabShifting;
};

#endif
