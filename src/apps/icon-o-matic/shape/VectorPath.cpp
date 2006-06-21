/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "VectorPath.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Message.h>
#include <TypeConstants.h>

#include <agg_basics.h>
#include <agg_bounding_rect.h>
#include <agg_conv_curve.h>
#include <agg_curves.h>
#include <agg_math.h>

#include "support.h"

#define obj_new(type, n)		((type *)malloc ((n) * sizeof(type)))
#define obj_renew(p, type, n)	((type *)realloc (p, (n) * sizeof(type)))
#define obj_free				free

#define ALLOC_CHUNKS 20

// get_path_storage
bool
get_path_storage(agg::path_storage& path,
				 const control_point* points, int32 count, bool closed)
{
	if (count > 1) {
		path.move_to(points[0].point.x,
					 points[0].point.y);

		for (int32 i = 1; i < count; i++) {
			path.curve4(points[i - 1].point_out.x,
						points[i - 1].point_out.y,
						points[i].point_in.x,
						points[i].point_in.y,
						points[i].point.x,
						points[i].point.y);
		}
		if (closed) {
			// curve from last to first control point
			path.curve4(points[count - 1].point_out.x,
						points[count - 1].point_out.y,
						points[0].point_in.x,
						points[0].point_in.y,
						points[0].point.x,
						points[0].point.y);
			path.close_polygon();
		} else {
			// straight line from last to first control point
			path.line_to(points[0].point.x,
						 points[0].point.y);
			path.close_polygon();
		}

		return true;
	}
	return false;
}


// constructor
VectorPath::VectorPath()
	: BArchivable(),
	  Observable(),
	  Referenceable(),
	  fPath(NULL),
	  fClosed(false),
	  fPointCount(0),
	  fAllocCount(0),
	  fCachedBounds(0.0, 0.0, -1.0, -1.0)
{
}

// constructor
VectorPath::VectorPath(const VectorPath& from)
	: BArchivable(),
	  Observable(),
	  Referenceable(),
	  fPath(NULL),
	  fPointCount(0),
	  fAllocCount(0),
	  fCachedBounds(0.0, 0.0, -1.0, -1.0)
{
	*this = from;
}

// constructor
VectorPath::VectorPath(const BMessage* archive)
	: BArchivable(),
	  Observable(),
	  Referenceable(),
	  fPath(NULL),
	  fClosed(false),
	  fPointCount(0),
	  fAllocCount(0),
	  fCachedBounds(0.0, 0.0, -1.0, -1.0)
{
	if (archive) {
		type_code typeFound;
		int32 countFound;
		if (archive->GetInfo("point", &typeFound, &countFound) >= B_OK
			&& typeFound == B_POINT_TYPE && _SetPointCount(countFound)) {
			memset(fPath, 0, fAllocCount * sizeof(control_point));
			BPoint point;
			BPoint pointIn;
			BPoint pointOut;
			bool connected;
			for (int32 i = 0; i < fPointCount
							  && archive->FindPoint("point", i, &point) >= B_OK
							  && archive->FindPoint("point in", i, &pointIn) >= B_OK
							  && archive->FindPoint("point out", i, &pointOut) >= B_OK
							  && archive->FindBool("connected", i, &connected) >= B_OK; i++) {
				fPath[i].point = point;
				fPath[i].point_in = pointIn;
				fPath[i].point_out = pointOut;
				fPath[i].connected = connected;
			}
		}
		if (archive->FindBool("path closed", &fClosed) < B_OK) {
			fClosed = false;
		}
	}
}

// destructor
VectorPath::~VectorPath()
{
	if (fPath)
		obj_free(fPath);
}

// operator=
VectorPath&
VectorPath::operator=(const VectorPath& from)
{
	_SetPointCount(from.fPointCount);
	fClosed = from.fClosed;
	if (fPath) {
		memcpy(fPath, from.fPath, fPointCount * sizeof(control_point));
		fCachedBounds = from.fCachedBounds;
	} else {
		fprintf(stderr, "VectorPath() -> allocation failed in operator=!\n");
		fAllocCount = 0;
		fPointCount = 0;
		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);
	}
	
	return *this;
}

// MakeEmpty
void
VectorPath::MakeEmpty()
{
	_SetPointCount(0);
}

// Archive
status_t
VectorPath::Archive(BMessage* into, bool deep) const
{
	status_t ret = BArchivable::Archive(into, deep);
	if (ret >= B_OK) {
		if (fPointCount > 0) {
			// improve BMessage efficency by preallocating storage for all points
			// with the first call
			ret = into->AddData("point", B_POINT_TYPE, &fPath[0].point,
								sizeof(BPoint), true, fPointCount);
			if (ret >= B_OK)
				ret = into->AddData("point in", B_POINT_TYPE, &fPath[0].point_in,
									sizeof(BPoint), true, fPointCount);
			if (ret >= B_OK)
				ret = into->AddData("point out", B_POINT_TYPE, &fPath[0].point_out,
									sizeof(BPoint), true, fPointCount);
			if (ret >= B_OK)
				ret = into->AddData("connected", B_BOOL_TYPE, &fPath[0].connected,
									sizeof(bool), true, fPointCount);
			// add the rest of the points
			for (int32 i = 1; i < fPointCount && ret >= B_OK; i++) {
				ret = into->AddData("point", B_POINT_TYPE, &fPath[i].point, sizeof(BPoint));
				if (ret >= B_OK)
					ret = into->AddData("point in", B_POINT_TYPE, &fPath[i].point_in, sizeof(BPoint));
				if (ret >= B_OK)
					ret = into->AddData("point out", B_POINT_TYPE, &fPath[i].point_out, sizeof(BPoint));
				if (ret >= B_OK)
					ret = into->AddData("connected", B_BOOL_TYPE, &fPath[i].connected, sizeof(bool));
			}
		}
		
		if (ret >= B_OK) {
			ret = into->AddBool("path closed", fClosed);
		} else {
			fprintf(stderr, "failed adding points!\n");
		}
		if (ret < B_OK) {
			fprintf(stderr, "failed adding closed!\n");
		}
		// finish off
		if (ret < B_OK) {
			ret = into->AddString("class", "VectorPath");
		}
	}
	return ret;
}

// AddPoint
bool
VectorPath::AddPoint(BPoint point)
{
	int32 index = fPointCount;

	if (_SetPointCount(fPointCount + 1)) {
		_SetPoint(index, point);
		Notify();
		return true;
	}

	return false;
}

// AddPoint
bool
VectorPath::AddPoint(BPoint point, int32 index)
{
	if (index < 0)
		index = 0;
	if (index > fPointCount)
		index = fPointCount;

	if (_SetPointCount(fPointCount + 1)) {
		// handle insert
		if (index < fPointCount - 1) {
			for (int32 i = fPointCount; i > index; i--) {
				fPath[i].point = fPath[i - 1].point;
				fPath[i].point_in = fPath[i - 1].point_in;
				fPath[i].point_out = fPath[i - 1].point_out;
				fPath[i].connected = fPath[i - 1].connected;
			}
		}
		_SetPoint(index, point);
		Notify();
		return true;
	}
	return false;
}

// RemovePoint
bool
VectorPath::RemovePoint(int32 index)
{
	if (index >= 0 && index < fPointCount) {

		if (index < fPointCount - 1) {
			// move points
			for (int32 i = index; i < fPointCount - 1; i++) {
				fPath[i].point = fPath[i + 1].point;
				fPath[i].point_in = fPath[i + 1].point_in;
				fPath[i].point_out = fPath[i + 1].point_out;
				fPath[i].connected = fPath[i + 1].connected;
			}
		}
		fPointCount -= 1;

		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

		Notify();
		return true;
	}
	return false;
}

// SetPoint
bool
VectorPath::SetPoint(int32 index, BPoint point)
{
	if (index == fPointCount)
		index = 0;
	if (index >= 0 && index < fPointCount) {
		BPoint offset = point - fPath[index].point;
		fPath[index].point = point;
		fPath[index].point_in += offset;
		fPath[index].point_out += offset;

		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

		Notify();
		return true;
	}
	return false;
}

// SetPoint
bool
VectorPath::SetPoint(int32 index, BPoint point,
								  BPoint pointIn, BPoint pointOut,
								  bool connected)
{
	if (index == fPointCount)
		index = 0;
	if (index >= 0 && index < fPointCount) {
		fPath[index].point = point;
		fPath[index].point_in = pointIn;
		fPath[index].point_out = pointOut;
		fPath[index].connected = connected;

		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

		Notify();
		return true;
	}
	return false;
}

// SetPointIn
bool
VectorPath::SetPointIn(int32 i, BPoint point)
{
	if (i == fPointCount)
		i = 0;
	if (i >= 0 && i < fPointCount) {
		// first, set the "in" point
		fPath[i].point_in = point;
		// now see what to do about the "out" point
		if (fPath[i].connected) {
			// keep all three points in one line
			BPoint v = fPath[i].point - fPath[i].point_in;
			float distIn = sqrtf(v.x * v.x + v.y * v.y);
			if (distIn > 0.0) {
				float distOut = point_point_distance(fPath[i].point, fPath[i].point_out);
				float scale = (distIn + distOut) / distIn;
				v.x *= scale;
				v.y *= scale;
				fPath[i].point_out = fPath[i].point_in + v;
			}
		}

		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

		Notify();
		return true;
	}
	return false;
}

// SetPointOut
bool
VectorPath::SetPointOut(int32 i, BPoint point, bool mirrorDist)
{
	if (i == fPointCount)
		i = 0;
	if (i >= 0 && i < fPointCount) {
		// first, set the "out" point
		fPath[i].point_out = point;
		// now see what to do about the "out" point
		if (mirrorDist) {
			// mirror "in" point around main control point
			BPoint v = fPath[i].point - fPath[i].point_out;
			fPath[i].point_in = fPath[i].point + v;
		} else if (fPath[i].connected) {
			// keep all three points in one line
			BPoint v = fPath[i].point - fPath[i].point_out;
			float distOut = sqrtf(v.x * v.x + v.y * v.y);
			if (distOut > 0.0) {
				float distIn = point_point_distance(fPath[i].point, fPath[i].point_in);
				float scale = (distIn + distOut) / distOut;
				v.x *= scale;
				v.y *= scale;
				fPath[i].point_in = fPath[i].point_out + v;
			}
		}

		fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

		Notify();
		return true;
	}
	return false;
}

// SetInOutConnected
bool
VectorPath::SetInOutConnected(int32 index, bool connected)
{
	if (index >= 0 && index < fPointCount) {
		fPath[index].connected = connected;
		Notify();
		return true;
	}
	return false;	
}

// GetPointAt
bool
VectorPath::GetPointAt(int32 index, BPoint& point) const
{
	if (index == fPointCount)
		index = 0;
	if (index >= 0 && index < fPointCount) {
		point = fPath[index].point;
		return true;
	}
	return false;
}

// GetPointInAt
bool
VectorPath::GetPointInAt(int32 index, BPoint& point) const
{
	if (index == fPointCount)
		index = 0;
	if (index >= 0 && index < fPointCount) {
		point = fPath[index].point_in;
		return true;
	}
	return false;
}

// GetPointOutAt
bool
VectorPath::GetPointOutAt(int32 index, BPoint& point) const
{
	if (index == fPointCount)
		index = 0;
	if (index >= 0 && index < fPointCount) {
		point = fPath[index].point_out;
		return true;
	}
	return false;
}

// GetPointsAt
bool
VectorPath::GetPointsAt(int32 index, BPoint& point,
						BPoint& pointIn, BPoint& pointOut, bool* connected) const
{
	if (index >= 0 && index < fPointCount) {
		point = fPath[index].point;
		pointIn = fPath[index].point_in;
		pointOut = fPath[index].point_out;

		if (connected)
			*connected = fPath[index].connected;

		return true;
	}
	return false;
}

// CountPoints
int32
VectorPath::CountPoints() const
{
	return fPointCount;
}

// distance_to_curve
static float
distance_to_curve(const BPoint& p, const BPoint& a, const BPoint& aOut, const BPoint& bIn, const BPoint& b)
{
	agg::curve4_inc curve(a.x, a.y, aOut.x, aOut.y,
						  bIn.x, bIn.y, b.x, b.y);

	float segDist = FLT_MAX;
	double x1, y1, x2, y2;
	unsigned cmd = curve.vertex(&x1, &y1);
	while (!agg::is_stop(cmd)) {
		cmd = curve.vertex(&x2, &y2);
		// first figure out if point is between segment start and end points
		double a = agg::calc_distance(p.x, p.y, x2, y2);
		double b = agg::calc_distance(p.x, p.y, x1, y1);

		float currentDist = min_c(a, b);

		if (a > 0.0 && b > 0.0) {
			double c = agg::calc_distance(x1, y1, x2, y2);

			double alpha = acos((b*b + c*c - a*a) / (2*b*c));
			double beta = acos((a*a + c*c - b*b) / (2*a*c));
	
			if (alpha <= PI2 && beta <= PI2) {
				currentDist = fabs(point_line_distance(x1, y1, x2, y2, p.x, p.y));
			}
		}

		if (currentDist < segDist) {
			segDist = currentDist;
		}
		x1 = x2;
		y1 = y2;
	}
	return segDist;
}

// GetDistance
bool
VectorPath::GetDistance(BPoint p, float* distance, int32* index) const
{
	if (fPointCount > 1) {
		// generate a curve for each segment of the path
		// then	iterate over the segments of the curve measuring the distance
		*distance = FLT_MAX;

		for (int32 i = 0; i < fPointCount - 1; i++) {
			float segDist = distance_to_curve(p,
											  fPath[i].point,
											  fPath[i].point_out,
											  fPath[i + 1].point_in,
											  fPath[i + 1].point);
			if (segDist < *distance) {
				*distance = segDist;
				*index = i + 1;
			}
		}
		if (fClosed) {
			float segDist = distance_to_curve(p,
											  fPath[fPointCount - 1].point,
											  fPath[fPointCount - 1].point_out,
											  fPath[0].point_in,
											  fPath[0].point);
			if (segDist < *distance) {
				*distance = segDist;
				*index = fPointCount;
			}
		}
		return true;
	}
	return false;
}

// FindBezierScale
bool
VectorPath::FindBezierScale(int32 index, BPoint point, double* scale) const
{
	if (index >= 0 && index < fPointCount && scale) {

		int maxStep = 1000;  
	
		double t = 0.0;
		double dt = 1.0 / maxStep;

		*scale = 0.0;
		double min = FLT_MAX;

		BPoint curvePoint;
		for (int step = 1; step < maxStep; step++) {
			t += dt;

			GetPoint(index, t, curvePoint);
			double d = point_point_distance(curvePoint, point);

			if (d < min) {
				min = d;
				*scale = t;
			}
		}
		return true;
	}
	return false;
}

// GetPoint
bool
VectorPath::GetPoint(int32 index, double t, BPoint& point) const
{
	if (index >= 0 && index < fPointCount) {

		double t1 = (1 - t) * (1 - t) * (1 - t);
		double t2 = (1 - t) * (1 - t) * t * 3;
		double t3 = (1 - t) * t * t * 3;
		double t4 = t * t * t;
   	
		if (index < fPointCount - 1) {
			point.x = fPath[index].point.x * t1 +
	   				  fPath[index].point_out.x * t2 +
	   				  fPath[index + 1].point_in.x * t3 +
	   				  fPath[index + 1].point.x * t4;
	
			point.y = fPath[index].point.y * t1 +
					  fPath[index].point_out.y * t2 +
					  fPath[index + 1].point_in.y * t3 +
					  fPath[index + 1].point.y * t4;
		} else if (fClosed) {
			point.x = fPath[fPointCount - 1].point.x * t1 +
	   				  fPath[fPointCount - 1].point_out.x * t2 +
	   				  fPath[0].point_in.x * t3 +
	   				  fPath[0].point.x * t4;
	
			point.y = fPath[fPointCount - 1].point.y * t1 +
					  fPath[fPointCount - 1].point_out.y * t2 +
					  fPath[0].point_in.y * t3 +
					  fPath[0].point.y * t4;
		}

		return true;
	}
	return false;
}

// SetClosed
void
VectorPath::SetClosed(bool closed)
{
	if (fClosed != closed) {
		fClosed = closed;
		Notify();
	}
}

// Bounds
BRect
VectorPath::Bounds() const
{
	// the bounds of the actual curves, not the control points!
	if (!fCachedBounds.IsValid())
		 fCachedBounds = _Bounds();
	return fCachedBounds;
}

// Bounds
BRect
VectorPath::_Bounds() const
{
	agg::path_storage path;

	BRect b;
	if (get_path_storage(path, fPath, fPointCount, fClosed)) {

		agg::conv_curve<agg::path_storage> curve(path);

		uint32 pathID[1];
		pathID[0] = 0;
		double left, top, right, bottom;

		agg::bounding_rect(curve, pathID, 0, 1, &left, &top, &right, &bottom);

		b.Set(left, top, right, bottom);
	} else if (fPointCount == 1) {
		b.Set(fPath[0].point.x, fPath[0].point.y, fPath[0].point.x, fPath[0].point.y);
	} else {
		b.Set(0.0, 0.0, -1.0, -1.0);
	}
	return b;
}

// ControlPointBounds
BRect
VectorPath::ControlPointBounds() const
{
	if (fPointCount > 0) {
		BRect r(fPath[0].point, fPath[0].point);
		for (int32 i = 0; i < fPointCount; i++) {
			// include point
			r.left = min_c(r.left, fPath[i].point.x);
			r.top = min_c(r.top, fPath[i].point.y);
			r.right = max_c(r.right, fPath[i].point.x);
			r.bottom = max_c(r.bottom, fPath[i].point.y);
			// include "in" point
			r.left = min_c(r.left, fPath[i].point_in.x);
			r.top = min_c(r.top, fPath[i].point_in.y);
			r.right = max_c(r.right, fPath[i].point_in.x);
			r.bottom = max_c(r.bottom, fPath[i].point_in.y);
			// include "out" point
			r.left = min_c(r.left, fPath[i].point_out.x);
			r.top = min_c(r.top, fPath[i].point_out.y);
			r.right = max_c(r.right, fPath[i].point_out.x);
			r.bottom = max_c(r.bottom, fPath[i].point_out.y);
		}
		return r;
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// Iterate
void
VectorPath::Iterate(Iterator* iterator, float smoothScale) const
{
	if (fPointCount > 1) {
		// generate a curve for each segment of the path
		// then	iterate over the segments of the curve
		agg::curve4_inc curve;
		curve.approximation_scale(smoothScale);

		for (int32 i = 0; i < fPointCount - 1; i++) {
iterator->MoveTo(fPath[i].point);
			curve.init(fPath[i].point.x, fPath[i].point.y,
					   fPath[i].point_out.x, fPath[i].point_out.y,
					   fPath[i + 1].point_in.x, fPath[i + 1].point_in.y,
					   fPath[i + 1].point.x, fPath[i + 1].point.y);

			double x, y;
			unsigned cmd = curve.vertex(&x, &y);
			while (!agg::is_stop(cmd)) {
				BPoint p(x, y);
				iterator->LineTo(p);
				cmd = curve.vertex(&x, &y);
			}
		}
		if (fClosed) {
iterator->MoveTo(fPath[fPointCount - 1].point);
			curve.init(fPath[fPointCount - 1].point.x, fPath[fPointCount - 1].point.y,
					   fPath[fPointCount - 1].point_out.x, fPath[fPointCount - 1].point_out.y,
					   fPath[0].point_in.x, fPath[0].point_in.y,
					   fPath[0].point.x, fPath[0].point.y);

			double x, y;
			unsigned cmd = curve.vertex(&x, &y);
			while (!agg::is_stop(cmd)) {
				BPoint p(x, y);
				iterator->LineTo(p);
				cmd = curve.vertex(&x, &y);
			}
		}
	}
}

// CleanUp
void
VectorPath::CleanUp()
{
	if (fPointCount == 0)
		return;

	bool notify = false;

	// remove last point if it is coincident with the first
	if (fClosed && fPointCount >= 1) {
		if (fPath[0].point == fPath[fPointCount - 1].point) {
			fPath[0].point_in = fPath[fPointCount - 1].point_in;
			_SetPointCount(fPointCount - 1);
			notify = true;
		}
	}

	for (int32 i = 0; i < fPointCount; i++) {
		// check for unnecessary, duplicate points
		if (i > 0) {
			if (fPath[i - 1].point == fPath[i].point &&
				fPath[i - 1].point == fPath[i - 1].point_out &&
				fPath[i].point == fPath[i].point_in) {
				// the previous point can be removed
				BPoint in = fPath[i - 1].point_in;
				if (RemovePoint(i - 1)) {
					i--;
					fPath[i].point_in = in;
					notify = true;
				}
			}
		}
		// re-establish connections of in-out control points if
		// they line up with the main control point
		if (fPath[i].point_in == fPath[i].point_out ||
			fPath[i].point == fPath[i].point_out ||
			fPath[i].point == fPath[i].point_in ||
			(fabs(point_line_distance(fPath[i].point_in.x, fPath[i].point_in.y,
									  fPath[i].point.x, fPath[i].point.y,
									  fPath[i].point_out.x, fPath[i].point_out.y)) < 0.01 &&
			 fabs(point_line_distance(fPath[i].point_out.x, fPath[i].point_out.y,
									  fPath[i].point.x, fPath[i].point.y,
									  fPath[i].point_in.x, fPath[i].point_in.y)) < 0.01)) {

			fPath[i].connected = true;
			notify = true;
		}
	}

	if (notify)
		Notify();
}

// Reverse
void
VectorPath::Reverse()
{
	VectorPath temp(*this);
	int32 index = 0;
	for (int32 i = fPointCount - 1; i >= 0; i--) {
		temp.SetPoint(index, fPath[i].point,
							 fPath[i].point_out,
							 fPath[i].point_in,
							 fPath[i].connected);
		index++;
	}
	*this = temp;

	Notify();
}

// PrintToStream
void
VectorPath::PrintToStream() const
{
	for (int32 i = 0; i < fPointCount; i++) {
		printf("point %ld: (%f, %f) -> (%f, %f) -> (%f, %f) (%d)\n", i,
				fPath[i].point_in.x, fPath[i].point_in.y,
				fPath[i].point.x, fPath[i].point.y,
				fPath[i].point_out.x, fPath[i].point_out.y,
				fPath[i].connected);
	}
}

// GetAGGPathStorage
bool
VectorPath::GetAGGPathStorage(agg::path_storage& path) const
{
	return get_path_storage(path, fPath, fPointCount, fClosed);
}

// _SetPoint
void
VectorPath::_SetPoint(int32 index, BPoint point)
{
	fPath[index].point = point;
	fPath[index].point_in = point;
	fPath[index].point_out = point;

	fPath[index].connected = true;

	fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);
}

// _SetPointCount
bool
VectorPath::_SetPointCount(int32 count)
{
	// handle reallocation if we run out of room
	if (count >= fAllocCount) {
		fAllocCount = ((count) / ALLOC_CHUNKS + 1) * ALLOC_CHUNKS;
		if (fPath) {
			fPath = obj_renew(fPath, control_point, fAllocCount);
		} else {
			fPath = obj_new(control_point, fAllocCount);
		}
		memset(fPath + fPointCount, 0, (fAllocCount - fPointCount) * sizeof(control_point));
	}
	// update point count
	if (fPath) {
		fPointCount = count;
	} else {
		// reallocation might have failed
		fPointCount = 0;
		fAllocCount = 0;
		fprintf(stderr, "VectorPath::_SetPointCount(%ld) - allocation failed!\n", count);
	}

	fCachedBounds.Set(0.0, 0.0, -1.0, -1.0);

	return fPath != NULL;
}
