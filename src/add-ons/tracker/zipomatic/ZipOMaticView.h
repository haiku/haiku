#ifndef __ZIPPO_VIEW_H__
#define __ZIPPO_VIEW_H__

#include <Box.h>
#include <Button.h>
#include <StringView.h>

class Activity;

class ZippoView : public BBox
{
	public:
			ZippoView	(BRect frame);

		virtual void	Draw (BRect frame);
		virtual void	AllAttached (void);
		virtual void	FrameMoved (BPoint a_point);
		virtual void	FrameResized (float a_width, float a_height);

	BButton *  m_stop_button;
	Activity * m_activity_view;

	BStringView * m_archive_name_view;
	BStringView * m_zip_output_view;

	private:

	
};

#endif // __ZIPPO_VIEW_H__
