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


namespace BPrivate {
class AboutView;
}
class BBitmap;
class BPoint;

class BAboutWindow : public BWindow {
public:
							BAboutWindow(const char* appName,
								const char* signature);
	virtual					~BAboutWindow();

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

			void			AddText(const char* header,
								const char** contents = NULL);

			BBitmap*		Icon();
	virtual	void			SetIcon(BBitmap* icon);

			const char*		Name();
	virtual	void			SetName(const char* name);

			const char*		Version();
	virtual	void			SetVersion(const char* version);

private:
	virtual	void			_ReservedAboutWindow20();
	virtual	void			_ReservedAboutWindow19();
	virtual	void			_ReservedAboutWindow18();
	virtual	void			_ReservedAboutWindow17();
	virtual	void			_ReservedAboutWindow16();
	virtual	void			_ReservedAboutWindow15();
	virtual	void			_ReservedAboutWindow14();
	virtual	void			_ReservedAboutWindow13();
	virtual	void			_ReservedAboutWindow12();
	virtual	void			_ReservedAboutWindow11();
	virtual	void			_ReservedAboutWindow10();
	virtual	void			_ReservedAboutWindow9();
	virtual	void			_ReservedAboutWindow8();
	virtual	void			_ReservedAboutWindow7();
	virtual	void			_ReservedAboutWindow6();
	virtual	void			_ReservedAboutWindow5();
	virtual	void			_ReservedAboutWindow4();
	virtual	void			_ReservedAboutWindow3();
	virtual	void			_ReservedAboutWindow2();
	virtual	void			_ReservedAboutWindow1();

private:
			BPrivate::AboutView* fAboutView;

			// FBC Padding
			uint32			_reserved[20];
};


#endif	// B_ABOUT_WINDOW_H
