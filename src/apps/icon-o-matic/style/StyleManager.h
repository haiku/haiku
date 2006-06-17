/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef STYLE_MANAGER_H
#define STYLE_MANAGER_H

#include <List.h>

#include "RWLocker.h"

class Style;

class StyleManagerListener {
 public:
								StyleManagerListener();
	virtual						~StyleManagerListener();

	virtual	void				StyleAdded(Style* style) = 0;
	virtual	void				StyleRemoved(Style* style) = 0;
};

class StyleManager : public RWLocker {
 public:
								StyleManager();
	virtual						~StyleManager();

			bool				AddStyle(Style* style);
			bool				RemoveStyle(Style* style);
			Style*				RemoveStyle(int32 index);

			void				MakeEmpty();

			int32				CountStyles() const;
			bool				HasStyle(Style* style) const;

			Style*				StyleAt(int32 index) const;
			Style*				StyleAtFast(int32 index) const;

	virtual	void				StateChanged();

			bool				AddListener(StyleManagerListener* listener);
			bool				RemoveListener(StyleManagerListener* listener);

 private:
			void				_StateChanged();
			void				_MakeEmpty();

			void				_NotifyStyleAdded(Style* style) const;
			void				_NotifyStyleRemoved(Style* style) const;

			BList				fStyles;
			BList				fListeners;
};

#endif // STYLE_MANAGER_H
