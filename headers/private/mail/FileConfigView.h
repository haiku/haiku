/*
 * Copyright 2004-2012, Haiku, Inc. All rights reserved.
 * Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _FILE_CONFIG_VIEW_H
#define _FILE_CONFIG_VIEW_H


#include <View.h>
#include <FilePanel.h>


class BButton;
class BMailAddOnSettings;
class BTextControl;


namespace BPrivate {


class FileControl : public BView {
public:
								FileControl(const char* name, const char* label,
									const char* pathOfFile = NULL,
									uint32 flavors = B_DIRECTORY_NODE);
	virtual						~FileControl();

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			void				SetText(const char* pathOfFile);
			const char*			Text() const;

			void				SetEnabled(bool enabled);

private:
			BTextControl*		fText;
			BButton*			fButton;

			BFilePanel*			fPanel;

			uint32				_reserved[5];
};


class MailFileConfigView : public FileControl {
public:
								MailFileConfigView(const char* label,
									const char* name, bool useMeta = false,
									const char* defaultPath = NULL,
									uint32 flavors = B_DIRECTORY_NODE);

			void				SetTo(const BMessage* archive,
									BMessage* metadata);
			status_t			SaveInto(BMailAddOnSettings& settings) const;

private:
			BMessage*			fMeta;
			bool				fUseMeta;
			const char*			fName;

			uint32				_reserved[5];
};


}	// namespace BPrivate


#endif	// _FILE_CONFIG_VIEW_H
