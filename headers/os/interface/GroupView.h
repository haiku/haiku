/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_GROUP_VIEW_H
#define	_GROUP_VIEW_H


#include <GroupLayout.h>
#include <View.h>


class BGroupView : public BView {
public:
								BGroupView(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								BGroupView(const char* name,
									enum orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								BGroupView(BMessage* from);
	virtual						~BGroupView();

	virtual	void				SetLayout(BLayout* layout);
			BGroupLayout*		GroupLayout() const;

	static	BArchivable*		Instantiate(BMessage* from);

private:

	// forbidden methods
								BGroupView(const BGroupView&);
			void				operator =(const BGroupView&);
};


#endif	// _GROUP_VIEW_H
