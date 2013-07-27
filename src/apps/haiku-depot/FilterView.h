/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FILTER_VIEW_H
#define FILTER_VIEW_H

#include <GroupView.h>


class BMenuField;
class BTextControl;


class FilterView : public BGroupView {
public:
								FilterView();
	virtual						~FilterView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

private:
			BMenuField*			fShowField;
			BMenuField*			fRepositoryField;
			BTextControl*		fSearchTermsText;
};

#endif // FILTER_VIEW_H
