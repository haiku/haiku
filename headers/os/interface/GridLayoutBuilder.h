/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GRID_LAYOUT_BUILDER_H
#define	_GRID_LAYOUT_BUILDER_H

#include <GridView.h>

class BGridLayoutBuilder {
public:
								BGridLayoutBuilder(
									float horizontalSpacing = 0.0f,
									float verticalSpacing = 0.0f);
								BGridLayoutBuilder(BGridLayout* layout);
								BGridLayoutBuilder(BGridView* view);

			BGridLayout*		GridLayout() const;
			BView*				View() const;
			BGridLayoutBuilder& GetGridLayout(BGridLayout** _layout);
			BGridLayoutBuilder& GetView(BView** _view);

			BGridLayoutBuilder& Add(BView* view, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
			BGridLayoutBuilder& Add(BLayoutItem* item, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

			BGridLayoutBuilder& SetColumnWeight(int32 column, float weight);
			BGridLayoutBuilder& SetRowWeight(int32 row, float weight);

			BGridLayoutBuilder& SetInsets(float left, float top, float right,
									float bottom);

								operator BGridLayout*();
								operator BView*();

private:
			BGridLayout*		fLayout;
};

#endif	// _GRID_LAYOUT_BUILDER_H
