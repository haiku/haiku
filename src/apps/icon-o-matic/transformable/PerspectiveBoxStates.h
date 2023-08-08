/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PERSPECTIVE_BOX_STATES_H
#define PERSPECTIVE_BOX_STATES_H

#include <Point.h>


class BView;
class PerspectiveBox;


namespace PerspectiveBoxStates {

class DragState {
 public:
									DragState(PerspectiveBox* parent);
	virtual							~DragState() {}

	virtual	void					SetOrigin(BPoint origin)
										{ fOrigin = origin; }
	virtual	void					DragTo(BPoint current, uint32 modifiers) = 0;
	virtual	void					UpdateViewCursor(BView* view, BPoint current) const = 0;

 protected:
			void					_SetViewCursor(BView* view,
												   const uchar* cursorData) const;

			BPoint					fOrigin;
			PerspectiveBox*	fParent;
};


class DragCornerState : public DragState {
 public:
								DragCornerState(
									PerspectiveBox* parent, BPoint* point);
	virtual						~DragCornerState() {}

	virtual	void				SetOrigin(BPoint origin);
	virtual	void				DragTo(BPoint current, uint32 modifiers);
	virtual	void				UpdateViewCursor(BView* view, BPoint current) const;

 private:
			BPoint*				fPoint;

			BPoint				fOldOffset;
};

} // PerspectiveBoxStates namespace

#endif // PERSPECTIVE_BOX_STATES_H
