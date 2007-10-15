/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef DEFAULT_MEDIA_THEME_H
#define DEFAULT_MEDIA_THEME_H


#include <MediaTheme.h>

class BParameterGroup;


namespace BPrivate {

class DefaultMediaTheme : public BMediaTheme {
	public:
		DefaultMediaTheme();

		virtual	BControl* MakeControlFor(BParameter* parameter);

		static BControl* MakeViewFor(BParameter* parameter, const BRect* hintRect = NULL);
			// this is also called by the BMediaTheme::MakeFallbackViewFor()
			// method - that's why it's a static.

	protected:
		virtual	BView* MakeViewFor(BParameterWeb* web, const BRect* hintRect = NULL);

	private:
		BView* MakeViewFor(BParameterGroup& group, const BRect* hintRect);
		BView* MakeSelfHostingViewFor(BParameter& parameter, const BRect* hintRect);
};

}	// namespace BPrivate

#endif	/* DEFAULT_MEDIA_THEME_H */
