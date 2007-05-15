/*
 * Copyright 2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef WIDGET_LAYOUT_TEST_VIEW_H
#define WIDGET_LAYOUT_TEST_VIEW_H


#include <Alignment.h>
#include <List.h>
#include <Rect.h>


class BView;


class View {
public:
								View();
								View(BRect frame);
	virtual						~View();

	virtual	void				SetFrame(BRect frame);
			BRect				Frame() const;
			BRect				Bounds() const;

			void				SetLocation(BPoint location);
			BPoint				Location() const;

			void				SetSize(BSize size);
			BSize				Size() const;

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

			BPoint				LocationInContainer() const;
			BRect				FrameInContainer() const;

			BPoint				ConvertFromContainer(BPoint point) const;
			BRect				ConvertFromContainer(BRect rect) const;
			BPoint				ConvertToContainer(BPoint point) const;
			BRect				ConvertToContainer(BRect rect) const;

			View*				Parent() const;
			BView*				Container() const;

			bool				AddChild(View* child);
			bool				RemoveChild(View* child);
			View*				RemoveChild(int32 index);

			int32				CountChildren() const;
			View*				ChildAt(int32 index) const;
			View*				ChildAt(BPoint point) const;
			View*				AncestorAt(BPoint point,
									BPoint* localPoint = NULL) const;
			int32				IndexOfChild(View* child) const;

			void				Invalidate(BRect rect);
			void				Invalidate();
	virtual	void				InvalidateLayout();
			bool				IsLayoutValid() const;

			void				SetViewColor(rgb_color color);

	virtual	void				Draw(BView* container, BRect updateRect);

	virtual	void				MouseDown(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseUp(BPoint where, uint32 buttons,
									int32 modifiers);
	virtual	void				MouseMoved(BPoint where, uint32 buttons,
									int32 modifiers);

	virtual	void				AddedToContainer();
	virtual	void				RemovingFromContainer();

	virtual	void				FrameChanged(BRect oldFrame, BRect newFrame);

	virtual	void				Layout();

protected:
			void				_AddedToParent(View* parent);
			void				_RemovingFromParent();

			void				_AddedToContainer(BView* container);
			void				_RemovingFromContainer();

			void				_Draw(BView* container, BRect updateRect);

private:
			BRect				fFrame;
			BView*				fContainer;
			View*				fParent;
			BList				fChildren;
			rgb_color			fViewColor;
			bool				fLayoutValid;
};

#endif	// WIDGET_LAYOUT_TEST_VIEW_H
