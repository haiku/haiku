/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PREFLET_VIEW_H
#define _PREFLET_VIEW_H

#include <View.h>

class BIconRule;

class SettingsHost;

class PrefletView : public BView {
public:
						PrefletView(SettingsHost* host);

	virtual	void		AttachedToWindow();
	virtual	void		MessageReceived(BMessage* message);

			void		Select(int32 index);
			BView*		CurrentPage();
			int32		CountPages() const;
			BView*		PageAt(int32 index);

private:
			BIconRule*	fRule;
			BView*		fPagesView;
};

#endif // PREFLETVIEW_H
