/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <View.h>


class ConfigView : public BView {
	public:
		ConfigView(uint32 flags = B_WILL_DRAW);	
		virtual ~ConfigView();
};

#endif	/* CONFIG_VIEW_H */
