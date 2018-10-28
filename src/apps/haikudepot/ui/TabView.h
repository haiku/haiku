/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TABVIEW_H
#define TABVIEW_H


#include <Messenger.h>
#include <TabView.h>


class TabView : public BTabView {
public:
	TabView(const BMessenger& target, const BMessage& message,
			const char* name = "tab view")
		:
		BTabView(name, B_WIDTH_FROM_WIDEST),
		fTarget(target),
		fMessage(message)
	{
	}

	virtual void Select(int32 tabIndex)
	{
		BTabView::Select(tabIndex);

		BMessage message(fMessage);
		message.AddInt32("tab index", tabIndex);
		fTarget.SendMessage(&message);
	}

private:
	BMessenger	fTarget;
	BMessage	fMessage;
};


#endif // TABVIEW_H
