/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PANEL_H
#define PANEL_H

#include <Window.h>

class Panel : public BWindow {
 public:
								Panel(BRect frame,
									  const char* title,
									  window_type type,
									  uint32 flags,
									  uint32 workspace = B_CURRENT_WORKSPACE);
								Panel(BRect frame,
									  const char* title,
									  window_look look,
									  window_feel feel,
									  uint32 flags,
									  uint32 workspace = B_CURRENT_WORKSPACE);
	virtual						~Panel();

	// Panel
	virtual void				Cancel();

 private:
			void				_InstallFilter();

};

#endif // PANEL_H
