/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef STATUS_MENU_FIELD_H
#define STATUS_MENU_FIELD_H


#include <MenuField.h>
#include <MenuItem.h>
#include <String.h>


class BBitmap;


class StatusMenuItem : public BMenuItem {
public:
								StatusMenuItem(const char* name, BMessage* message = NULL);
								StatusMenuItem(BMessage* archive);

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				DrawContent();
	virtual	void				GetContentSize(float* _width, float* _height);

			BBitmap*			Icon();
	virtual	void				SetIcon(BBitmap* icon);

			BRect				IconRect();
			BSize				IconSize();
			float				Spacing();

private:
			BBitmap*			fIcon;
};


class StatusMenuField : public BMenuField {
public:
								StatusMenuField(const char*, BMenu*);
								~StatusMenuField();

	virtual	void				SetDuplicate(bool on);
	virtual	void				SetUnmatched(bool on);

			BString				Status() { return fStatus; };
	virtual	void				ClearStatus();
	virtual	void				SetStatus(BString status);

protected:
	virtual	void				ShowStopIcon(bool show);
	virtual	void				ShowWarnIcon(bool show);

private:
			void				_FillIcons();
			BRect				_IconRect();

private:
			BString				fStatus;

			BBitmap*			fStopIcon;
			BBitmap*			fWarnIcon;
};


#endif	// STATUS_MENU_FIELD_H
