/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPLIT_VIEW_H
#define	_SPLIT_VIEW_H

#include <View.h>

class BSplitLayout;


class BSplitView : public BView {
public:
								BSplitView(
									enum orientation orientation = B_HORIZONTAL,
									float spacing = 0.0f);
	virtual						~BSplitView();

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(enum orientation orientation);

			float				SplitterSize() const;
			void				SetSplitterSize(float size);


			void				SetCollapsible(bool collapsible);
			void				SetCollapsible(int32 index, bool collapsible);
			void				SetCollapsible(int32 first, int32 last,
									bool collapsible);

//			void				AddChild(BView* child);
			void				AddChild(BView* child, BView* sibling = NULL);
			bool				AddChild(BView* child, float weight);
			bool				AddChild(int32 index, BView* child,
									float weight);

			bool				AddChild(BLayoutItem* child);
			bool				AddChild(BLayoutItem* child, float weight);
			bool				AddChild(int32 index, BLayoutItem* child,
									float weight);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);


	virtual	void				SetLayout(BLayout* layout);
									// overridden to avoid use

protected:
	virtual	void				DrawSplitter(BRect frame,
									enum orientation orientation, bool pressed);

private:
	static	void				_DrawDefaultSplitter(BView* view, BRect frame,
									enum orientation orientation, bool pressed);

private:
			BSplitLayout*		fSplitLayout;
};


#endif	// _SPLIT_VIEW_H
