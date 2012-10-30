/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <FileConfigView.h>


class ConfigView : public BView {
public:
								ConfigView();
			void				SetTo(const BMessage* archive);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;

private:
			BPrivate::MailFileConfigView* fFileView;
			BTextControl*		fTagControl;
};


#endif	// CONFIG_VIEW_H
