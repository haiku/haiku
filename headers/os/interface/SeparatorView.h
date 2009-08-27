/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SEPARATOR_VIEW_H
#define _SEPARATOR_VIEW_H


#include <Alignment.h>
#include <String.h>
#include <View.h>


class BSeparatorView : public BView {
public:
								BSeparatorView(enum orientation orientation,
									border_style border = B_PLAIN_BORDER);
								BSeparatorView(const char* name,
									const char* label,
									enum orientation orientation
										= B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								BSeparatorView(const char* name,
									BView* labelView,
									enum orientation orientation
										= B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								BSeparatorView(const char* label = NULL,
									enum orientation orientation
										= B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));
								BSeparatorView(BView* labelView,
									enum orientation orientation
										= B_HORIZONTAL,
									border_style border = B_FANCY_BORDER,
									const BAlignment& alignment
										= BAlignment(B_ALIGN_HORIZONTAL_CENTER,
											B_ALIGN_VERTICAL_CENTER));

								BSeparatorView(BMessage* archive);

	virtual						~BSeparatorView();

	static 	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	virtual	void				Draw(BRect updateRect);

	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

			void				SetOrientation(enum orientation orientation);
			void				SetAlignment(const BAlignment& aligment);
			void				SetBorderStyle(border_style border);

			void				SetLabel(const char* label);
			void				SetLabel(BView* view, bool deletePrevious);

	virtual status_t			Perform(perform_code code, void* data);

protected:
	virtual	void				DoLayout();

private:
	// FBC padding
	virtual	void				_ReservedSeparatorView1();
	virtual	void				_ReservedSeparatorView2();
	virtual	void				_ReservedSeparatorView3();
	virtual	void				_ReservedSeparatorView4();
	virtual	void				_ReservedSeparatorView5();
	virtual	void				_ReservedSeparatorView6();
	virtual	void				_ReservedSeparatorView7();
	virtual	void				_ReservedSeparatorView8();
	virtual	void				_ReservedSeparatorView9();
	virtual	void				_ReservedSeparatorView10();

private:
			void				_Init(const char* label, BView* labelView,
									enum orientation orientation,
									BAlignment alignment, border_style border);

			float				_BorderSize() const;
			BRect				_MaxLabelBounds() const;

private:
			BString				fLabel;
			BView*				fLabelView;

			orientation			fOrientation;
			BAlignment			fAlignment;
			border_style		fBorder;

			uint32				_reserved[10];
};

#endif // _SEPARATOR_VIEW_H
