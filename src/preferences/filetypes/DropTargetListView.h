/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DROP_TARGET_LIST_VIEW_H
#define DROP_TARGET_LIST_VIEW_H


#include <ListView.h>


class DropTargetListView : public BListView {
	public:
		DropTargetListView(const char* name,
			list_view_type type = B_SINGLE_SELECTION_LIST,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE);
		virtual ~DropTargetListView();

		virtual void Draw(BRect updateRect);
		virtual void MouseMoved(BPoint where, uint32 transit,
			const BMessage* dragMessage);

		virtual bool AcceptsDrag(const BMessage* message);

	private:
		void _InvalidateFrame();

		bool	fDropTarget;
};

#endif	// DROP_TARGET_LIST_VIEW_H
