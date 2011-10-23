/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_INVALIDATOR_H
#define	_LAYOUT_INVALIDATOR_H


class BLayout;
class BLayoutItem;
class BView;


struct BLayoutInvalidator {
	static	void				InvalidateLayout(BLayout* layout, bool deep);
	static	void				InvalidateLayout(BLayoutItem* item, bool deep);
	static	void				InvalidateLayout(BView* view, bool deep);
};

#endif
