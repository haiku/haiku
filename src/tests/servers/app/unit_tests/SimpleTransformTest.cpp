/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SimpleTransform.h"
#include "IntPoint.h"
#include "IntRect.h"

#include <Gradient.h>
#include <Point.h>
#include <Region.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class SimpleTransformTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(SimpleTransformTest);
	CPPUNIT_TEST(TransformPoint);
	CPPUNIT_TEST(TransformIntPoint);
	CPPUNIT_TEST(TransformRect);
	CPPUNIT_TEST(TransformIntRect);
	CPPUNIT_TEST(TransformRegion);
	CPPUNIT_TEST(TransformGradientLinear);
	CPPUNIT_TEST(TransformGradientRadial);
	CPPUNIT_TEST(TransformGradientRadialFocus);
	CPPUNIT_TEST(TransformGradientDiamond);
	CPPUNIT_TEST(TransformGradientConic);
	CPPUNIT_TEST(TransformPointArray);
	CPPUNIT_TEST(TransformRectArray);
	CPPUNIT_TEST(TransformRegionArray);
	CPPUNIT_TEST_SUITE_END();

public:
	void TransformPoint()
	{
		BPoint point(1.0, 1.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 10.0);
		specimen.Apply(&point);
		CPPUNIT_ASSERT(point == BPoint(11.0, 11.0));

		specimen.AddOffset(10.0, 10.0);
		point.Set(0.0, 0.0);
		specimen.Apply(&point);
		CPPUNIT_ASSERT(point == BPoint(20.0, 20.0));
	}

	void TransformIntPoint()
	{
		IntPoint point(1, 1);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 10.0);
		specimen.Apply(&point);
		CPPUNIT_ASSERT(point == BPoint(11.0, 11.0));
	}

	void TransformRect()
	{
		BRect rect(5.0, 10.0, 15.0, 20.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 10.0);
		specimen.Apply(&rect);
		CPPUNIT_ASSERT(rect == BRect(15.0, 20.0, 25.0, 30.0));

		specimen.SetScale(2.5);
		specimen.Apply(&rect);
		CPPUNIT_ASSERT(rect == BRect(47.5, 60.0, 72.5, 85.0));
	}

	void TransformIntRect()
	{
		IntRect rect(5, 10, 15, 20);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 10.0);
		specimen.Apply(&rect);
		CPPUNIT_ASSERT(rect == IntRect(15, 20, 25, 30));

		specimen.SetScale(2);
		specimen.Apply(&rect);
		CPPUNIT_ASSERT(rect == BRect(40, 50, 60, 70));
	}

	void TransformRegion()
	{
		BRegion region;
		region.Include(BRect( 5.0,  5.0, 20.0, 20.0));
		region.Include(BRect(10.0, 10.0, 30.0, 30.0));
		region.Exclude(BRect(10.0, 20.0, 20.0, 25.0));

		BRegion reference1 = region;
		reference1.OffsetBy(10, 20);

		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.Apply(&region);
		CPPUNIT_ASSERT(region == reference1);

		specimen.SetScale(2.5);

		BRegion reference2;
		reference2.Include(BRect(47.0, 82.0, 87.0, 122.0));
		reference2.Include(BRect(60.0, 95.0, 112.0, 147.0));
		reference2.Exclude(BRect(60.0, 120.0, 86.0, 134.0));

		specimen.Apply(&region);
		CPPUNIT_ASSERT(region == reference2);
	}

	void TransformGradientLinear()
	{
		BGradientLinear gradient(10.0, 20.0, 30.0, 40.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&gradient);
		CPPUNIT_ASSERT(gradient.Start() == BPoint(35.0, 70.0));
		CPPUNIT_ASSERT(gradient.End()   == BPoint(85.0, 120.0));
	}

	void TransformGradientRadial()
	{
		BGradientRadial gradient(10.0, 20.0, 10.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&gradient);
		CPPUNIT_ASSERT(gradient.Center() == BPoint(35.0, 70.0));
	}

	void TransformGradientRadialFocus()
	{
		BGradientRadialFocus gradient(10.0, 20.0, 10.0, 30.0, 40.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&gradient);
		CPPUNIT_ASSERT(gradient.Center() == BPoint(35.0, 70.0));
		CPPUNIT_ASSERT(gradient.Focal()  == BPoint(85.0, 120.0));
	}

	void TransformGradientDiamond()
	{
		BGradientDiamond gradient(10.0, 20.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&gradient);
		CPPUNIT_ASSERT(gradient.Center() == BPoint(35.0, 70.0));
	}

	void TransformGradientConic()
	{
		BGradientConic gradient(10.0, 20.0, 10.0);
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&gradient);
		CPPUNIT_ASSERT(gradient.Center() == BPoint(35.0, 70.0));
	}

	void TransformPointArray()
	{
		BPoint points[3];
		points[0].Set(10.0, 20.0);
		points[1].Set(30.0, 40.0);
		points[2].Set(50.0, 60.0);
		BPoint transformedPoints[3];
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.Apply(&transformedPoints[0], &points[0], 3);
		CPPUNIT_ASSERT(transformedPoints[0] == BPoint(20.0, 40.0));
		CPPUNIT_ASSERT(transformedPoints[1] == BPoint(40.0, 60.0));
		CPPUNIT_ASSERT(transformedPoints[2] == BPoint(60.0, 80.0));
	}

	void TransformRectArray()
	{
		BRect rects[3];
		rects[0].Set( 5.0, 10.0, 15.0, 20.0);
		rects[1].Set(15.0, 20.0, 25.0, 30.0);
		rects[2].Set(25.0, 30.0, 35.0, 40.0);
		BRect transformedRects[3];
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&transformedRects[0], &rects[0], 3);
		CPPUNIT_ASSERT(transformedRects[0] == BRect(22.5, 45.0, 47.5, 70.0));
		CPPUNIT_ASSERT(transformedRects[1] == BRect(47.5, 70.0, 72.5, 95.0));
		CPPUNIT_ASSERT(transformedRects[2] == BRect(72.5, 95.0, 97.5, 120.0));
	}

	void TransformRegionArray()
	{
		BRegion regions[2];
		regions[0].Include(BRect( 5.0,  5.0, 20.0, 20.0));
		regions[0].Include(BRect(10.0, 10.0, 30.0, 30.0));
		regions[0].Exclude(BRect(10.0, 20.0, 20.0, 25.0));
		regions[1].Include(BRect( 5.0,  5.0, 20.0, 20.0));
		regions[1].Include(BRect(10.0, 10.0, 30.0, 30.0));
		regions[1].Exclude(BRect(10.0, 20.0, 20.0, 25.0));
		BRegion transformedRegions[3];
		SimpleTransform specimen;
		specimen.AddOffset(10.0, 20.0);
		specimen.SetScale(2.5);
		specimen.Apply(&transformedRegions[0], &regions[0], 2);
		BRegion reference;
		reference.Include(BRect(22.0, 32.0, 62.0, 72.0));
		reference.Include(BRect(35.0, 45.0, 87.0, 97.0));
		reference.Exclude(BRect(35.0, 70.0, 61.0, 84.0));

		CPPUNIT_ASSERT(transformedRegions[0] == reference);
		CPPUNIT_ASSERT(transformedRegions[1] == reference);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(SimpleTransformTest, getTestSuiteName());
