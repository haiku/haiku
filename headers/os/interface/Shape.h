/*
 * Copyright 2006-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHAPE_H
#define _SHAPE_H


#include <Archivable.h>

class BPoint;
class BRect;
class BShape;

namespace BPrivate {
	class ServerLink;
	class PicturePlayer;
};


class BShapeIterator {
public:
								BShapeIterator();
	virtual						~BShapeIterator();

	virtual	status_t			IterateMoveTo(BPoint* point);
	virtual	status_t			IterateLineTo(int32 lineCount,
									BPoint* linePts);
	virtual	status_t			IterateBezierTo(int32 bezierCount,
									BPoint* bezierPts);
	virtual	status_t			IterateClose();

	virtual	status_t			IterateArcTo(float& rx, float& ry,
									float& angle, bool largeArc,
									bool counterClockWise, BPoint& point);

			status_t			Iterate(BShape* shape);

private:
	virtual	void				_ReservedShapeIterator2();
	virtual	void				_ReservedShapeIterator3();
	virtual	void				_ReservedShapeIterator4();

			uint32				reserved[4];
};


class BShape : public BArchivable {
public:
								BShape();
								BShape(const BShape& other);
								BShape(BMessage* archive);
	virtual						~BShape();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

			BShape&				operator=(const BShape& other);

			bool				operator==(const BShape& other) const;
			bool				operator!=(const BShape& other) const;

			void				Clear();
			BRect				Bounds() const;
			BPoint				CurrentPosition() const;

			status_t			AddShape(const BShape* other);

			status_t			MoveTo(BPoint point);
			status_t			LineTo(BPoint linePoint);
			status_t			BezierTo(BPoint controlPoints[3]);
			status_t			BezierTo(const BPoint& control1,
									const BPoint& control2,
									const BPoint& endPoint);
			status_t			ArcTo(float rx, float ry,
									float angle, bool largeArc,
									bool counterClockWise,
									const BPoint& point);
			status_t			Close();

private:
	// FBC padding
	virtual status_t			Perform(perform_code code, void* data);

	virtual	void				_ReservedShape1();
	virtual	void				_ReservedShape2();
	virtual	void				_ReservedShape3();
	virtual	void				_ReservedShape4();

private:
	friend class BShapeIterator;
	friend class BView;
	friend class BFont;
	friend class BPrivate::PicturePlayer;
	friend class BPrivate::ServerLink;

			void				GetData(int32* opCount, int32* ptCount,
									uint32** opList, BPoint** ptList);
			void				SetData(int32 opCount, int32 ptCount,
									const uint32* opList,
									const BPoint* ptList);
			void				InitData();
			bool				AllocatePts(int32 count);
			bool				AllocateOps(int32 count);

private:
			uint32				fState;
			uint32				fBuildingOp;
			void*				fPrivateData;

			uint32				reserved[4];
};

#endif // _SHAPE_H
