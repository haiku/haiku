/*
 * Copyright (c) 2008-2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Artur Wyszynski <harakash@gmail.com>
 */


#include <View.h>
#include <Gradient.h>


class GradientsView : public BView {
public:
							GradientsView(const BRect &r);
	virtual					~GradientsView(void);

	virtual void			Draw(BRect update);
			void			DrawLinear(BRect update);
			void			DrawRadial(BRect update);
			void			DrawRadialFocus(BRect update);
			void			DrawDiamond(BRect update);
			void			DrawConic(BRect update);
			void			SetType(BGradient::Type type);

private:
			BGradient::Type	fType;
};
