/*
 * Copyright 2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <View.h>


class ConfigView : public BView {
public:
							ConfigView(uint32 flags = B_WILL_DRAW);
	virtual 				~ConfigView();
};


#endif	/* CONFIG_VIEW_H */
