/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_CONTEXT_H
#define	_LAYOUT_CONTEXT_H

#include <List.h>

class BLayoutContext;


class BLayoutContextListener {
public:
								BLayoutContextListener();
	virtual						~BLayoutContextListener();

	virtual	void				LayoutContextLeft(BLayoutContext* context) = 0;
};


class BLayoutContext {
public:
								BLayoutContext();
								~BLayoutContext();

			void				AddListener(BLayoutContextListener* listener);
			void				RemoveListener(
									BLayoutContextListener* listener);

private:
			BList				fListeners;
};

#endif	//	_LAYOUT_CONTEXT_H
