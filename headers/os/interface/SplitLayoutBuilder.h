/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPLIT_LAYOUT_BUILDER_H
#define	_SPLIT_LAYOUT_BUILDER_H

#include <SplitView.h>

class BSplitLayoutBuilder {
public:
								BSplitLayoutBuilder(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								BSplitLayoutBuilder(BSplitView* view);

			BSplitView*			SplitView() const;
			BSplitLayoutBuilder& GetSplitView(BSplitView** view);

			BSplitLayoutBuilder& Add(BView* view);
			BSplitLayoutBuilder& Add(BView* view, float weight);
			BSplitLayoutBuilder& Add(BLayoutItem* item);
			BSplitLayoutBuilder& Add(BLayoutItem* item, float weight);

			BSplitLayoutBuilder& SetCollapsible(bool collapsible);

			BSplitLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator BSplitView*();

private:
			BSplitView*			fView;
};

#endif	// _SPLIT_LAYOUT_BUILDER_H
