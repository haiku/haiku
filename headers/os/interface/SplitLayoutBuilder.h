/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPLIT_LAYOUT_BUILDER_H
#define	_SPLIT_LAYOUT_BUILDER_H

#include <SplitView.h>

class BSplitLayoutBuilder {
public:
								BSplitLayoutBuilder(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = 0.0f);
								BSplitLayoutBuilder(BSplitView* view);

			BSplitView*			SplitView() const;
			BSplitLayoutBuilder& GetSplitView(BSplitView** view);

			BSplitLayoutBuilder& Add(BView* view);
			BSplitLayoutBuilder& Add(BView* view, float weight);
			BSplitLayoutBuilder& Add(BLayoutItem* item);
			BSplitLayoutBuilder& Add(BLayoutItem* item, float weight);

			BSplitLayoutBuilder& SetCollapsible(bool collapsible);

								operator BSplitView*();

private:
			BSplitView*			fView;
};

#endif	// _SPLIT_LAYOUT_BUILDER_H
