/*
 * Copyright (C) 2019 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef ATTRIBUTESVIEW_H
#define ATTRIBUTESVIEW_H


#include <ColumnListView.h>
#include <GroupView.h>

#include "Model.h"


namespace BPrivate {

class AttributesView: public BGroupView
{
public:
						AttributesView(Model* model);

private:
	BColumnListView*	fListView;	
};

};


#endif /* !ATTRIBUTESVIEW_H */
