#ifndef TOGGLE_SCROLL_VIEW_H
#define TOGGLE_SCROLL_VIEW_H

#include <ScrollView.h>

class ToggleScrollView : public BView {
public:
						ToggleScrollView(const char * name, BView * target,
							uint32 flags = 0,
							bool horizontal = false, bool vertical = false,
							border_style border = B_FANCY_BORDER,
							bool auto_hide_horizontal = false,
							bool auto_hide_vertical = false);
virtual					~ToggleScrollView();
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	BScrollBar		*ScrollBar(orientation flag) const;
// extension to BScrollView API
virtual void			ToggleScrollBar(bool horizontal = false, 
										bool vertical = false);

virtual	void			SetBorder(border_style border);
virtual border_style	Border() const;

virtual	status_t		SetBorderHighlighted(bool state);
virtual bool			IsBorderHighlighted() const;

virtual void			SetTarget(BView *new_target);
virtual BView			*Target() const;

virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);

virtual void			ResizeToPreferred();
virtual void			GetPreferredSize(float *width, float *height);
virtual void			MakeFocus(bool state = true);

// overloaded functions
virtual void			SetFlags(uint32 flags);
virtual void			SetResizingMode(uint32 mode);

private:
static	BView *			ResizeTarget(BView * target, bool horizontal, bool vertical);

		const char *	_name;
		BView *			_target;
		uint32			_flags;
		bool			_horizontal;
		bool			_vertical;
		border_style	_border;
		bool			_auto_hide_horizontal;
		bool			_auto_hide_vertical;
		BScrollView *	fScrollView;

};
		
#endif // TOGGLE_SCROLL_VIEW_H
