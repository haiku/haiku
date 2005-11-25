/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */
#ifndef TERM_BASE_VIEW_H
#define TERM_BASE_VIEW_H


#include <View.h>


class TermView;


class TermBaseView : public BView {
	public:
		TermBaseView(BRect frame, TermView *);
		~TermBaseView();

	private:
		void SetBaseColor(rgb_color);
		virtual void FrameResized(float newWidth, float newHeight);

		TermView *fTermView;
};

#endif	/* TERM_BASE_VIEW_H */
