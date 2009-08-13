/*
 * Copyright 2007-2009 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef B_ABOUT_WINDOW_H
#define B_ABOUT_WINDOW_H


#include <String.h>


class BAboutWindow {
	public:
		BAboutWindow(const char *appName, int32 firstCopyrightYear,
			const char **authors, const char *extraInfo = NULL);
		virtual ~BAboutWindow();

		void Show();

	private:
		BString* 		fAppName;
		BString*		fText;
};

#endif	// B_ABOUT_WINDOW_H
