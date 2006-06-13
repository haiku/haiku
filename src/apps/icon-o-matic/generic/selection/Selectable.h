/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SELECTABLE_H
#define SELECTABLE_H

#include <SupportDefs.h>

class Selectable {
 public:
								Selectable();
	virtual						~Selectable();

	inline	bool				IsSelected() const
									{ return fSelected; }

	virtual	void				SelectedChanged() = 0;

 private:
	friend class Selection;
			void				SetSelected(bool selected);

			bool				fSelected;
};

#endif // SELECTABLE_H
