/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef CHANNEL_TRANSFORM_H
#define CHANNEL_TRANSFORM_H

#include "Transformable.h"

class ChannelTransform : public Transformable {
 public:
								ChannelTransform();
								ChannelTransform(const ChannelTransform& other);
	virtual						~ChannelTransform();

	// ChannelTransform
	virtual	void				Update(bool deep = true) {}

			void				SetTransformation(const Transformable& other);

			void				SetTransformation(BPoint pivot,
												  BPoint translation,
												  double rotation,
												  double xScale,
												  double yScale);

			void				SetPivot(BPoint pivot);

	virtual	void				TranslateBy(BPoint offset);
	virtual	void				RotateBy(BPoint origin, double degrees);
			void				RotateBy(double degrees);

	virtual	void				ScaleBy(BPoint origin, double xScale,
									double yScale);
			void				ScaleBy(double xScale, double yScale);

			void				SetTranslationAndScale(BPoint offset,
													   double xScale,
													   double yScale);

	virtual	void				Reset();

	inline	BPoint				Pivot() const
									{ return fPivot; }
	inline	BPoint				Translation() const
									{ return fTranslation; }
	inline	double				LocalRotation() const
									{ return fRotation; }
	inline	double				LocalXScale() const
									{ return fXScale; }
	inline	double				LocalYScale() const
									{ return fYScale; }

			ChannelTransform&	operator=(const ChannelTransform& other);

 private:
			void				_UpdateMatrix();

			BPoint				fPivot;
			BPoint				fTranslation;
			double				fRotation;
			double				fXScale;
			double				fYScale;
};

#endif // CHANNEL_TRANSFORM_H

