/*
	
	TimeView.h
	
*/

#ifndef TIME_VIEW_H
#define TIME_VIEW_H

#include <Box.h>

class TimeView : public BBox
{
	public:
		typedef BBox	inherited;

		TimeView(BRect frame);
		virtual void	Draw(BRect frame);
};

#endif
