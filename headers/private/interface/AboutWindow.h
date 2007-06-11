/*
 * Copyright 2007 Haiku, Inc.
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
		BAboutWindow(char *appName, int32 firstCopyrightYear,
			int32 numAuthors, const char **authors, char *extraInfo = NULL);
		virtual ~BAboutWindow();

		void Show();

	private:
		BString* 		fAppName;
		BString*		fText;
};

#endif	// B_ABOUT_WINDOW_H
