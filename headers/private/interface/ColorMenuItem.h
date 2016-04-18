/*
 * Copyright 2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef COLOR_MENU_ITEM_H
#define COLOR_MENU_ITEM_H


#include <InterfaceDefs.h>
#include <MenuItem.h>


class BColorMenuItem : public BMenuItem {
public:
								BColorMenuItem(const char* label,
									BMessage* message, rgb_color color,
									char shortcut = 0,
									uint32 modifiers = 0);
								BColorMenuItem(BMenu* menu, rgb_color color,
									BMessage* message = NULL);
								BColorMenuItem(BMessage* data);

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				DrawContent();
	virtual	void				GetContentSize(float* _width, float* _height);

	virtual	void				SetMarked(bool mark);

			rgb_color			Color() const { return fColor; };
	virtual	void				SetColor(rgb_color color) { fColor = color; };

private:
	virtual	void				_ReservedColorMenuItem1();
	virtual	void				_ReservedColorMenuItem2();
	virtual	void				_ReservedColorMenuItem3();
	virtual	void				_ReservedColorMenuItem4();
	virtual	void				_ReservedColorMenuItem5();
	virtual	void				_ReservedColorMenuItem6();
	virtual	void				_ReservedColorMenuItem7();
	virtual	void				_ReservedColorMenuItem8();
	virtual	void				_ReservedColorMenuItem9();
	virtual	void				_ReservedColorMenuItem10();
	virtual	void				_ReservedColorMenuItem11();
	virtual	void				_ReservedColorMenuItem12();
	virtual	void				_ReservedColorMenuItem13();
	virtual	void				_ReservedColorMenuItem14();
	virtual	void				_ReservedColorMenuItem15();
	virtual	void				_ReservedColorMenuItem16();
	virtual	void				_ReservedColorMenuItem17();
	virtual	void				_ReservedColorMenuItem18();
	virtual	void				_ReservedColorMenuItem19();
	virtual	void				_ReservedColorMenuItem20();

			float				_LeftMargin();
			float				_Padding();
			float				_ColorRectWidth();

private:
			rgb_color			fColor;

			uint32				_reserved[30];
};


#endif	// COLOR_MENU_ITEM_H
