/*
 * Copyright 2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef WORKSPACES_LAYER_H
#define WORKSPACES_LAYER_H


#include "ViewLayer.h"

class WindowLayer;


class WorkspacesLayer : public ViewLayer {
	public:
		WorkspacesLayer(BRect frame, const char* name, int32 token,
			uint32 resize, uint32 flags);
		virtual ~WorkspacesLayer();

		virtual	void Draw(DrawingEngine* drawingEngine,
						BRegion* effectiveClipping,
						BRegion* windowContentClipping,
						bool deep = false);
		virtual void MouseDown(BMessage* message, BPoint where, int32* _viewToken);

	private:
		void _GetGrid(int32& columns, int32& rows);
		BRect _WorkspaceAt(int32 i);
		BRect _WindowFrame(const BRect& workspaceFrame,
					const BRect& screenFrame, const BRect& windowFrame,
					BPoint windowPosition);

		void _DrawWindow(DrawingEngine* drawingEngine, const BRect& workspaceFrame,
					const BRect& screenFrame, WindowLayer* window,
					BPoint windowPosition, BRegion& backgroundRegion,
					bool active);
		void _DrawWorkspace(DrawingEngine* drawingEngine, int32 index);

		void _DarkenColor(RGBColor& color) const;
};

#endif	// WORKSPACES_LAYER_H
