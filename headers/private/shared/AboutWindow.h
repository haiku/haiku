/*
 * Copyright 2007-2012 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood <leavengood@gmail.com>
 *		John Scipione <jscipione@gmail.com>
 */
#ifndef B_ABOUT_WINDOW_H
#define B_ABOUT_WINDOW_H


#include <GroupView.h>
#include <Window.h>
#include <View.h>


class AboutView;
class BBitmap;
class BPoint;
class BHandler;

class BAboutWindow : public BWindow {
 public:
							BAboutWindow(const char* appName,
								const char* signature);
	virtual					~BAboutWindow();

	virtual	bool			QuitRequested();
	virtual	void			Show();

			BPoint			AboutPosition(float width, float height);
			void			AddDescription(const char* description);
			void			AddCopyright(int32 firstCopyrightYear,
								const char* copyrightHolder,
								const char** extraCopyrights = NULL);
			void			AddAuthors(const char** authors);
			void			AddSpecialThanks(const char** thanks);
			void			AddVersionHistory(const char** history);
			void			AddExtraInfo(const char* extraInfo);

			BBitmap*		Icon();
			void			SetIcon(BBitmap* icon);

			const char*		Name();
			void			SetName(const char* name);

			const char*		Version();
			void			SetVersion(const char* version);

 private:
			AboutView*		fAboutView;
			BHandler*		fCaller;
};

#endif	// B_ABOUT_WINDOW_H
