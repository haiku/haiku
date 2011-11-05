/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GRID_VIEW_H
#define	_GRID_VIEW_H

#include <GridLayout.h>
#include <View.h>


class BGridView : public BView {
public:
								BGridView(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								BGridView(const char* name,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
								BGridView(BMessage* from);
	virtual						~BGridView();

	virtual	void				SetLayout(BLayout* layout);
			BGridLayout*		GridLayout() const;

	static	BArchivable*		Instantiate(BMessage* from);

private:

	// forbidden methods
								BGridView(const BGridView&);
			void				operator =(const BGridView&);
};


#endif	// _GRID_VIEW_H
