/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CARD_VIEW_H
#define _CARD_VIEW_H

#include <View.h>

class BCardLayout;

class BCardView : public BView {
public:
								BCardView();
								BCardView(const char* name);
								BCardView(BMessage* from);
	virtual						~BCardView();

	virtual	void				SetLayout(BLayout* layout);
			BCardLayout*		CardLayout() const;

	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

private:

	// FBC padding - prevent breaking compatibility when methods are added
	virtual	void				_ReservedCardView1();
	virtual	void				_ReservedCardView2();
	virtual	void				_ReservedCardView3();
	virtual	void				_ReservedCardView4();
	virtual	void				_ReservedCardView5();
	virtual	void				_ReservedCardView6();
	virtual	void				_ReservedCardView7();
	virtual	void				_ReservedCardView8();
	virtual	void				_ReservedCardView9();
	virtual	void				_ReservedCardView10();

	// forbitten methods
								BCardView(const BCardView&);
			void				operator =(const BCardView&);

			uint32				_reserved[10];
};

#endif // _CARD_VIEW_H
