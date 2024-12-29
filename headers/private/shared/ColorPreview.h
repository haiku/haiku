/*
 * Copyright 2009-2024 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COLOR_PREVIEW_H
#define _COLOR_PREVIEW_H


#include <Control.h>


class BMessage;
class BMessageRunner;


namespace BPrivate {


class BColorPreview : public BControl {
public:
								BColorPreview(const char* name, const char* label,
									BMessage* message, uint32 flags = B_WILL_DRAW);
	virtual						~BColorPreview();

	virtual	void				Draw(BRect updateRect);
	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	void				MessageReceived(BMessage *message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit, const BMessage* message);
	virtual	void				MouseUp(BPoint where);

			rgb_color			Color() const;
			void				SetColor(rgb_color color);

private:
			void				_DragColor(BPoint where);

			rgb_color			fColor;

			BMessageRunner*		fMessageRunner;
};


} // namespace BPrivate


#endif // _COLOR_PREVIEW_H
