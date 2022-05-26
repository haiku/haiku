/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef FILTER_VIEW_H
#define FILTER_VIEW_H

#include <GroupView.h>


class BCheckBox;
class BMenu;
class BMenuField;
class BMenuItem;
class BTextControl;
class Model;


enum {
	MSG_CATEGORY_SELECTED		= 'ctsl',
	MSG_DEPOT_SELECTED			= 'dpsl',
	MSG_SEARCH_TERMS_MODIFIED	= 'stmd',
};


class FilterView : public BGroupView {
public:
								FilterView();
	virtual						~FilterView();

	virtual void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				AdoptModel(Model& model);

private:
	static	void				_AddCategoriesToMenu(Model& model, BMenu* menu);
	static	bool				_SelectCategoryCode(BMenu* menu,
									const BString& code);
	static	bool				_MatchesCategoryCode(BMenuItem* item,
									const BString& code);

private:
			BMenuField*			fShowField;
			BTextControl*		fSearchTermsText;
};

#endif // FILTER_VIEW_H
