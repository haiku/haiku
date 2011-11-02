/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_SPLIT_VIEW_H
#define	_SPLIT_VIEW_H

#include <View.h>

class BSplitLayout;


class BSplitView : public BView {
public:
								BSplitView(
									enum orientation orientation
										= B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								BSplitView(BMessage* from);
	virtual						~BSplitView();

			void				SetInsets(float left, float top, float right,
									float bottom);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float insets);
			void				GetInsets(float* left, float* top,
									float* right, float* bottom) const;

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(enum orientation orientation);

			float				SplitterSize() const;
			void				SetSplitterSize(float size);

			float				ItemWeight(int32 index) const;
			float				ItemWeight(BLayoutItem* item) const;
			void				SetItemWeight(int32 index, float weight,
									bool invalidateLayout);
			void				SetItemWeight(BLayoutItem* item, float weight);

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

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	void				DrawSplitter(BRect frame,
									const BRect& updateRect,
									enum orientation orientation,
									bool pressed);

private:

	// FBC padding
	virtual	void				_ReservedSplitView1();
	virtual	void				_ReservedSplitView2();
	virtual	void				_ReservedSplitView3();
	virtual	void				_ReservedSplitView4();
	virtual	void				_ReservedSplitView5();
	virtual	void				_ReservedSplitView6();
	virtual	void				_ReservedSplitView7();
	virtual	void				_ReservedSplitView8();
	virtual	void				_ReservedSplitView9();
	virtual	void				_ReservedSplitView10();

	static	void				_DrawDefaultSplitter(BView* view, BRect frame,
									const BRect& updateRect,
									enum orientation orientation,
									bool pressed);

private:
			BSplitLayout*		fSplitLayout;
			uint32				_reserved[3];
};


#endif	// _SPLIT_VIEW_H
