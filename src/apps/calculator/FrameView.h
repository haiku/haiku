/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef __FRAME_VIEW__
#define __FRAME_VIEW__

#include <View.h>

class _EXPORT FrameView : public BView
{
public:
							FrameView(BRect frame, const char *name,
									  uint32 resizeMask, int bevel_indent=1);
							FrameView(BMessage *data);
	virtual	status_t		Archive(BMessage *msg, bool deep) const;
	static 	BArchivable *	Instantiate(BMessage *archive);

	virtual	void			Draw(BRect updateRect);
	virtual	void			FrameResized(float new_width, float new_height);
	virtual void			AttachedToWindow();
			void			MouseDown(BPoint where);

			void 			ColoringBasis(rgb_color view_color);


	typedef struct
	{
		char	*label;
		uint32	message;
	} ClusterInfo;

							FrameView(const BPoint &where, const char *name,
									  uint32 resizeMask, int items_wide,
									  int items_high, int bevel, int key_border,
									  int key_height, int key_width,
									  const ClusterInfo button_info[]);
private:
			int		bevel_indent;
};

#endif
