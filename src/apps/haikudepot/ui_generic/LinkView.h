/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef LINK_VIEW_H
#define LINK_VIEW_H


#include <Invoker.h>
#include <StringView.h>


class LinkView : public BStringView, public BInvoker {
public:
								LinkView(const char* name, const char* string,
									BMessage* message);

	virtual void				AttachedToWindow();

	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseDown(BPoint where);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);
			void				SetEnabled(bool enabled);

private:
			void				_UpdateLinkColor();

private:

			bool				fEnabled;
			bool				fMouseInside;
};


#endif // LINK_VIEW_H
