/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FILTER_VIEW_H
#define FILTER_VIEW_H

#include <GroupView.h>


class BCheckBox;
class BMenuField;
class BTextControl;
class Model;


enum {
	MSG_CATEGORY_SELECTED		= 'ctsl',
	MSG_FILTER_SELECTED			= 'ftsl',
	MSG_DEPOT_SELECTED			= 'dpsl',
	MSG_SEARCH_TERMS_MODIFIED	= 'stmd',
};


class FilterView : public BGroupView {
public:
								FilterView();
	virtual						~FilterView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	virtual void				AdoptModel(const Model& model);
	virtual void				AdoptCheckmarks(const Model& model);

private:
			BMenuField*			fShowField;
			BMenuField*			fRepositoryField;
			BTextControl*		fSearchTermsText;

			BCheckBox*			fAvailableCheckBox;
			BCheckBox*			fInstalledCheckBox;
			BCheckBox*			fDevelopmentCheckBox;
			BCheckBox*			fSourceCodeCheckBox;
};

#endif // FILTER_VIEW_H
