/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICON_RULE_H
#define _ICON_RULE_H

#include <Invoker.h>
#include <View.h>

class BList;
class BMessage;

class BIconRule : public BView, public BInvoker {
public:
						BIconRule(const char* name);
	virtual				~BIconRule();

			status_t	Invoke(BMessage* message);

	virtual	void		AttachedToWindow();

	virtual	void		MouseDown(BPoint point);
	virtual	void		Draw(BRect updateRect);

			BMessage*	SelectionMessage() const;
			void		SetSelectionMessage(BMessage* message);

			void		AddIcon(const char* label, const BBitmap* icon);
			void		RemoveIconAt(int32 index);
			void		RemoveAllIcons();

			int32		CountIcons() const;

			void		SlideToIcon(int32 index);
			void		SlideToNext();
			void		SlideToPrevious();

			int32		IndexOf(BPoint point);

	virtual	BSize		MinSize();
	virtual	BSize		MaxSize();
	virtual	BSize		PreferredSize();

private:
			BList*		fIcons;
			int32		fSelIndex;
			BMessage*	fMessage;
};

#endif // _ICON_RULE_H
