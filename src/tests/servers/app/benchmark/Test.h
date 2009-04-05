/*
 * Copyright (C) 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef TEST_H
#define TEST_H

#include <Region.h>

class BView;

class Test {
public:
								Test();
	virtual						~Test();

	virtual	void				Prepare(BView* view) = 0;
	virtual	bool				RunIteration(BView* view) = 0;
	virtual	void				PrintResults(BView* view);

			void				SetupClipping(BView* view);

protected:
			BRegion				fClippingRegion;
};

#endif // TEST_H
