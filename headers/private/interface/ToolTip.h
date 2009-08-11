/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TOOL_TIP_H
#define _TOOL_TIP_H


#include <Alignment.h>
#include <Archivable.h>
#include <Point.h>
#include <Referenceable.h>


class BView;
class BTextView;


class BToolTip : public BArchivable, public BReferenceable {
public:
								BToolTip();
								BToolTip(BMessage* archive);
	virtual						~BToolTip();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	BView*				View() const = 0;

	virtual void				SetSticky(bool enable);
			bool				IsSticky() const;
	virtual void				SetMouseRelativeLocation(BPoint location);
			BPoint				MouseRelativeLocation() const;
	virtual void				SetAlignment(BAlignment alignment);
			BAlignment			Alignment() const;

	virtual	void				AttachedToWindow();
	virtual void				DetachedFromWindow();

protected:
			bool				Lock();
			void				Unlock();

private:
								BToolTip(const BToolTip& other);
			BToolTip&			operator=(const BToolTip &other);

			void				_InitData();

private:
			bool				fLockedLooper;
			bool				fIsSticky;
			BPoint				fRelativeLocation;
			BAlignment			fAlignment;
};


class BTextToolTip : public BToolTip {
public:
								BTextToolTip(const char* text);
								BTextToolTip(BMessage* archive);
	virtual						~BTextToolTip();

	static	BTextToolTip*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	BView*				View() const;

			const char*			Text() const;
			void				SetText(const char* text);

private:
			void				_InitData(const char* text);

private:
			BTextView*			fTextView;
};


#endif	// _TOOL_TIP_H
