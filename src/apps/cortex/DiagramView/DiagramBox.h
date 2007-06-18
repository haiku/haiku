// DiagramBox.h (Cortex/DiagramView.h)
//
// * HISTORY
//   c.lenz		25sep99		Begun
//
#ifndef DIAGRAM_BOX_H
#define DIAGRAM_BOX_H

#include "cortex_defs.h"
#include "DiagramItem.h"
#include "DiagramItemGroup.h"

#include <Region.h>
#include <Window.h>


__BEGIN_CORTEX_NAMESPACE


class DiagramBox : public DiagramItem, public DiagramItemGroup {
	public:
		DiagramBox(BRect frame, uint32 flags = 0);
		virtual ~DiagramBox();

		virtual void DrawBox() = 0;
			// a hook functions that
			// is called from Draw() to do the actual drawing

		// derived from DiagramItemGroup

		virtual bool AddItem(DiagramItem *item);
		virtual bool RemoveItem(DiagramItem *item);

		// derived from DiagramItem

		// returns the Boxes frame rectangle
		BRect Frame() const
		{
			return fFrame;
		}

		void Draw(BRect updateRect);

		virtual void MouseDown(BPoint point, uint32 buttons, uint32 clicks);
		virtual void MouseOver(BPoint point, uint32 transit);
		
		virtual void MessageDragged(BPoint point, uint32 transit, const BMessage *message);
		virtual void MessageDropped(BPoint point, BMessage *message);

		void MoveBy(float x, float y, BRegion *updateRegion);
		virtual void ResizeBy(float horizontal, float vertical);
		
		enum flag_t {
			M_DRAW_UNDER_ENDPOINTS = 0x1
		};

	private:
		void _SetOwner(DiagramView *owner);

	private:
		BRect fFrame;
			// the boxes' frame rectangle

		// flags:
		// 	M_DRAW_UNDER_ENDPOINTS -  don't remove EndPoint frames from
		//							the clipping region
		uint32 fFlags;
};

__END_CORTEX_NAMESPACE

#endif	// DIAGRAM_BOX_H
