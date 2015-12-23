/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SIMPLE_TRANSFORM_TEST_H
#define SIMPLE_TRANSFORM_TEST_H

#include <TestCase.h>
#include <TestSuite.h>

#include "SimpleTransform.h"


class SimpleTransformTest : public BTestCase {
public:
	static	void			AddTests(BTestSuite& parent);

			void			TransformPoint();
			void			TransformIntPoint();
			void			TransformRect();
			void			TransformIntRect();
			void			TransformRegion();

			void			TransformGradientLinear();
			void			TransformGradientRadial();
			void			TransformGradientRadialFocus();
			void			TransformGradientDiamond();
			void			TransformGradientConic();

			void			TransformPointArray();
			void			TransformRectArray();
			void			TransformRegionArray();
};


#endif // SIMPLE_TRANSFORM_TEST_H
