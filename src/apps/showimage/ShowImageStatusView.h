/*
 * Copyright 2003-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Fernando Francisco de Oliveira
 *		Michael Wilber
 *		Philippe Houdoin
 */
#ifndef SHOW_IMAGE_STATUS_VIEW_H
#define SHOW_IMAGE_STATUS_VIEW_H


#include <Entry.h>
#include <String.h>
#include <View.h>

class InfoStatusView;
class ProgressStatusView;


class ShowImageStatusView : public BView {
public:
						ShowImageStatusView(const char* name = "image_status");

	virtual	void		AttachedToWindow();

	void				SetBusy(bool busy = true);
	void				SetBusyText(const BString& text);
	void				SetBusyProgress(float progress);

	void				Update(const entry_ref& ref,
									const BString& text, const BString& pages,
									const BString& imageType, float zoom);
	void				SetZoom(float zoom);
private:
	bool				fBusy;

 	InfoStatusView*		fInfoStatusView;
 	ProgressStatusView* fProgressStatusView;
};


#endif	// SHOW_IMAGE_STATUS_VIEW_H
