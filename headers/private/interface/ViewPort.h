/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _VIEW_PORT_H
#define _VIEW_PORT_H


#include <View.h>


namespace BPrivate {


class BViewPort : public BView {
public:
								BViewPort(BView* child = NULL);
								BViewPort(BLayoutItem* child);
								BViewPort(const char* name,
									BView* child = NULL);
								BViewPort(const char* name,
									BLayoutItem* child);
	virtual						~BViewPort();

			BView*				ChildView() const;
			void				SetChildView(BView* child);

			BLayoutItem*		ChildItem() const;
			void				SetChildItem(BLayoutItem* child);

private:
			class ViewPortLayout;
			friend class ViewPortLayout;

private:
			void				_Init();

private:
			ViewPortLayout*		fLayout;
			BLayoutItem*		fChild;
};


}	// namespace BPrivate


using ::BPrivate::BViewPort;


#endif	// _VIEW_PORT_H
