/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef STYLE_MANAGER_H
#define STYLE_MANAGER_H


#include <List.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


class Style;

#ifdef ICON_O_MATIC
class StyleContainerListener {
 public:
								StyleContainerListener();
	virtual						~StyleContainerListener();

	virtual	void				StyleAdded(Style* style, int32 index) = 0;
	virtual	void				StyleRemoved(Style* style) = 0;
};
#endif // ICON_O_MATIC

class StyleContainer {
 public:
								StyleContainer();
	virtual						~StyleContainer();

			bool				AddStyle(Style* style);
			bool				AddStyle(Style* style, int32 index);
			bool				RemoveStyle(Style* style);
			Style*				RemoveStyle(int32 index);

			void				MakeEmpty();

			int32				CountStyles() const;
			bool				HasStyle(Style* style) const;
			int32				IndexOf(Style* style) const;

			Style*				StyleAt(int32 index) const;
			Style*				StyleAtFast(int32 index) const;

 private:
			BList				fStyles;

			void				_MakeEmpty();

#ifdef ICON_O_MATIC
 public:
			bool				AddListener(StyleContainerListener* listener);
			bool				RemoveListener(StyleContainerListener* listener);

 private:
			void				_NotifyStyleAdded(Style* style,
												  int32 index) const;
			void				_NotifyStyleRemoved(Style* style) const;

			BList				fListeners;
#endif // ICON_O_MATIC
};


_END_ICON_NAMESPACE


#endif	// STYLE_MANAGER_H
