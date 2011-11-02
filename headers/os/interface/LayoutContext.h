/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
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

private:
	virtual	void				_ReservedLayoutContextListener1();
	virtual	void				_ReservedLayoutContextListener2();
	virtual	void				_ReservedLayoutContextListener3();
	virtual	void				_ReservedLayoutContextListener4();
	virtual	void				_ReservedLayoutContextListener5();

			uint32				_reserved[3];
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
			uint32				_reserved[5];
};

#endif	//	_LAYOUT_CONTEXT_H
