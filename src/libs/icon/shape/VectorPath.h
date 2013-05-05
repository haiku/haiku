/*
 * Copyright 2006-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef VECTOR_PATH_H
#define VECTOR_PATH_H


#include "IconBuild.h"
#include "Transformable.h"

#include <agg_path_storage.h>

#include <Rect.h>
#include <String.h>

#ifdef ICON_O_MATIC
#	include "IconObject.h"

#	include <Archivable.h>
#	include <List.h>
#endif // ICON_O_MATIC

class BBitmap;
class BMessage;
class BView;


_BEGIN_ICON_NAMESPACE


struct control_point {
	BPoint		point;		// actual point on path
	BPoint		point_in;	// control point for incomming curve
	BPoint		point_out;	// control point for outgoing curve
	bool		connected;	// if all 3 points should be on one line
};

#ifdef ICON_O_MATIC
class PathListener {
 public:
								PathListener();
	virtual						~PathListener();

	virtual	void				PointAdded(int32 index) = 0;
	virtual	void				PointRemoved(int32 index) = 0;
	virtual	void				PointChanged(int32 index) = 0;
	virtual	void				PathChanged() = 0;
	virtual	void				PathClosedChanged() = 0;
	virtual	void				PathReversed() = 0;
};

class VectorPath : public BArchivable,
				   public IconObject {
#else
class VectorPath {
#endif // ICON_O_MATIC
 public:

	class Iterator {
	 public:
								Iterator() {}
		virtual					~Iterator() {}

		virtual	void			MoveTo(BPoint point) = 0;
		virtual	void			LineTo(BPoint point) = 0;
	};

								VectorPath();
								VectorPath(const VectorPath& from);
								VectorPath(BMessage* archive);

	virtual						~VectorPath();

#ifdef ICON_O_MATIC
	// IconObject
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	PropertyObject*		MakePropertyObject() const;
	virtual	bool				SetToPropertyObject(
									const PropertyObject* object);
#else
	inline	void				Notify() {}
#endif // ICON_O_MATIC

	// VectorPath
			VectorPath&			operator=(const VectorPath& from);
			bool				operator==(const VectorPath& from) const;

			void				MakeEmpty();

			bool				AddPoint(BPoint point);
			bool				AddPoint(const BPoint& point,
										 const BPoint& pointIn,
										 const BPoint& pointOut,
										 bool connected);
			bool				AddPoint(BPoint point, int32 index);

			bool				RemovePoint(int32 index);

								// modify existing points position
			bool				SetPoint(int32 index, BPoint point);
			bool				SetPoint(int32 index, BPoint point,
													  BPoint pointIn,
													  BPoint pointOut,
													  bool connected);
			bool				SetPointIn(int32 index, BPoint point);
			bool				SetPointOut(int32 index, BPoint point,
										   bool mirrorDist = false);

			bool				SetInOutConnected(int32 index, bool connected);

								// query existing points position
			bool				GetPointAt(int32 index, BPoint& point) const;
			bool				GetPointInAt(int32 index, BPoint& point) const;
			bool				GetPointOutAt(int32 index, BPoint& point) const;
			bool				GetPointsAt(int32 index,
											BPoint& point,
											BPoint& pointIn,
											BPoint& pointOut,
											bool* connected = NULL) const;

			int32				CountPoints() const;

#ifdef ICON_O_MATIC
								// iterates over curve segments and returns
								// the distance and index of the point that
								// started the segment that is closest
			bool				GetDistance(BPoint point,
											float* distance, int32* index) const;

								// at curve segment indicated by "index", this
								// function looks for the closest point
								// directly on the curve and returns a "scale"
								// that indicates the distance on the curve
								// between [0..1]
			bool				FindBezierScale(int32 index, BPoint point,
												double* scale) const;
								// this function can be used to get a point
								// directly on the segment indicated by "index"
								// "scale" is on [0..1] indicating the distance
								// from the start of the segment to the end
			bool				GetPoint(int32 index, double scale,
										 BPoint& point) const;
#endif // ICON_O_MATIC

			void				SetClosed(bool closed);
			bool				IsClosed() const
									{ return fClosed; }

			BRect				Bounds() const;
			BRect				ControlPointBounds() const;

			void				Iterate(Iterator* iterator,
										float smoothScale = 1.0) const;

			void				CleanUp();
			void				Reverse();
			void				ApplyTransform(const Transformable& transform);

			void				PrintToStream() const;

			bool				GetAGGPathStorage(agg::path_storage& path) const;

#ifdef ICON_O_MATIC
			bool				AddListener(PathListener* listener);
			bool				RemoveListener(PathListener* listener);
			int32				CountListeners() const;
			PathListener*		ListenerAtFast(int32 index) const;
#endif // ICON_O_MATIC


 private:
			BRect				_Bounds() const;
			void				_SetPoint(int32 index, BPoint point);
			void				_SetPoint(int32 index,
										  const BPoint& point,
										  const BPoint& pointIn,
										  const BPoint& pointOut,
										  bool connected);
			bool				_SetPointCount(int32 count);

#ifndef ICON_O_MATIC
	inline	void				_NotifyPointAdded(int32 index) const {}
	inline	void				_NotifyPointChanged(int32 index) const {}
	inline	void				_NotifyPointRemoved(int32 index) const {}
	inline	void				_NotifyPathChanged() const {}
	inline	void				_NotifyClosedChanged() const {}
	inline	void				_NotifyPathReversed() const {}
#else
			void				_NotifyPointAdded(int32 index) const;
			void				_NotifyPointChanged(int32 index) const;
			void				_NotifyPointRemoved(int32 index) const;
			void				_NotifyPathChanged() const;
			void				_NotifyClosedChanged() const;
			void				_NotifyPathReversed() const;

			BList				fListeners;
#endif // ICON_O_MATIC

			control_point*		fPath;
			bool				fClosed;

			int32				fPointCount;
			int32				fAllocCount;

	mutable	BRect				fCachedBounds;
};


_END_ICON_NAMESPACE


#endif	// VECTOR_PATH_H
