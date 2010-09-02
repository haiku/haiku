/*
 * Copyright 2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef INFO_VIEW_H
#define INFO_VIEW_H


#include <GridView.h>

class InfoView : public BGridView {
public:
    					InfoView();
    virtual 			~InfoView();

private:
			void		_AddString(const char* label, const char* value);
};

#endif /* INFO_VIEW_H */
