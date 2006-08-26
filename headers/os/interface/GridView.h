/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GRID_VIEW_H
#define	_GRID_VIEW_H

#include <GridLayout.h>
#include <View.h>


class BGridView : public BView {
public:
								BGridView(float horizontalSpacing = 0.0f,
									float verticalSpacing = 0.0f);
	virtual						~BGridView();

	virtual	void				SetLayout(BLayout* layout);

			BGridLayout*		GridLayout() const;
};


#endif	// _GRID_VIEW_H
