/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001, Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H


#include <MailSettingsView.h>

#include <FileConfigView.h>


class ConfigView : public BMailSettingsView {
public:
								ConfigView();

			void				SetTo(const BMailAddOnSettings& settings);

	virtual status_t			SaveInto(BMailAddOnSettings& settings) const;

private:
			BPrivate::MailFileConfigView* fFileView;
			BTextControl*		fTagControl;
};


#endif	// CONFIG_VIEW_H
