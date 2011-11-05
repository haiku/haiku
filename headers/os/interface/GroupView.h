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

	virtual	status_t			Perform(perform_code d, void* arg);

private:

	// FBC padding
	virtual	void				_ReservedGroupView1();
	virtual	void				_ReservedGroupView2();
	virtual	void				_ReservedGroupView3();
	virtual	void				_ReservedGroupView4();
	virtual	void				_ReservedGroupView5();
	virtual	void				_ReservedGroupView6();
	virtual	void				_ReservedGroupView7();
	virtual	void				_ReservedGroupView8();
	virtual	void				_ReservedGroupView9();
	virtual	void				_ReservedGroupView10();

	// forbidden methods
								BGroupView(const BGroupView&);
			void				operator =(const BGroupView&);

			uint32				_reserved[2];
};


#endif	// _GROUP_VIEW_H
