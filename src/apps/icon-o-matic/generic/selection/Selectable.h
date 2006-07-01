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

class Selection;

class Selectable {
 public:
								Selectable();
	virtual						~Selectable();

	inline	bool				IsSelected() const
									{ return fSelected; }

	virtual	void				SelectedChanged() = 0;

	// this works only if the Selection is known
			void				SetSelected(bool selected,
											bool exclusive = true);
			void				SetSelection(Selection* selection);

 private:
	friend class Selection;
			void				_SetSelected(bool selected);

			bool				fSelected;
			Selection*			fSelection;
};

#endif // SELECTABLE_H
