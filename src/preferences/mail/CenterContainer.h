#ifndef CENTER_CONTAINER_H
#define CENTER_CONTAINER_H
/* CenterContainer - a container which centers its contents in the middle
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <View.h>


class CenterContainer : public BView
{
	public:
		CenterContainer(BRect rect,bool centerHoriz = true);

		virtual void	AttachedToWindow();
		virtual void	AllAttached();
		virtual void	FrameResized(float width, float height);
		virtual void	GetPreferredSize(float *width, float *height);

		void			Layout();
		void			SetSpacing(float spacing);
		void			DeleteChildren();

	private:
		float			fSpacing, fWidth, fHeight;
		bool			fCenterHoriz;
};

#endif	/* CENTER_CONTAINER_H */
