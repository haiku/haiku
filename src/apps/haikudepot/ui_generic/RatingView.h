/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2018-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATING_VIEW_H
#define RATING_VIEW_H


#include <Referenceable.h>
#include <View.h>


class RatingView : public BView {
public:
								RatingView(const char* name);
	virtual						~RatingView();

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

			void				SetRating(float rating);
			float				Rating() const;

protected:
	virtual const BBitmap*		StarBitmap();

private:
			float				fRating;
};


#endif // RATING_VIEW_H
