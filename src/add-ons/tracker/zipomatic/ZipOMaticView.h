#ifndef __ZIPPO_VIEW_H__
#define __ZIPPO_VIEW_H__

#include <Box.h>
#include <Button.h>
#include <StringView.h>

#include "ZipOMaticActivity.h"


class ZippoView : public BBox
{
public:
							ZippoView(BRect frame);

	virtual	void			Draw(BRect frame);
	virtual	void			FrameMoved(BPoint point);
	virtual	void			FrameResized(float width, float height);

			BButton*		fStopButton;
			Activity*		fActivityView;
			BStringView*	fArchiveNameView;
			BStringView*	fZipOutputView;
};

#endif // __ZIPPO_VIEW_H__

