/*******************************************************************************
/
/	File:			Shape.h
/
/   Description:    BShape encapsulates a Postscript-style "path"
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _SHAPE_H
#define _SHAPE_H

#include <BeBuild.h>
#include <Archivable.h>

/*----------------------------------------------------------------*/
/*----- BShapeIterator class -------------------------------------*/

class BShapeIterator {

public:
						BShapeIterator();
virtual					~BShapeIterator();

virtual	status_t		IterateMoveTo(BPoint *point);
virtual	status_t		IterateLineTo(int32 lineCount, BPoint *linePts);
virtual	status_t		IterateBezierTo(int32 bezierCount, BPoint *bezierPts);
virtual	status_t		IterateClose();

		status_t		Iterate(BShape *shape);

private:

virtual	void			_ReservedShapeIterator1();
virtual	void			_ReservedShapeIterator2();
virtual	void			_ReservedShapeIterator3();
virtual	void			_ReservedShapeIterator4();

		uint32			reserved[4];
};

/*----------------------------------------------------------------*/
/*----- BShape class ---------------------------------------------*/

class BShape : public BArchivable {

public:
						BShape();
						BShape(const BShape &copyFrom);
						BShape(BMessage *data);
virtual					~BShape();

virtual	status_t		Archive(BMessage *into, bool deep = true) const;
static	BArchivable		*Instantiate(BMessage *data);

		void			Clear();
		BRect			Bounds() const;

		status_t		AddShape(const BShape *other);

		status_t		MoveTo(BPoint point);
		status_t		LineTo(BPoint linePoint);
		status_t		BezierTo(BPoint controlPoints[3]);
		status_t		Close();

/*----- Private or reserved ---------------*/
virtual status_t		Perform(perform_code d, void *arg);

private:

virtual	void			_ReservedShape1();
virtual	void			_ReservedShape2();
virtual	void			_ReservedShape3();
virtual	void			_ReservedShape4();

		friend class	BShapeIterator;
		friend class	TPicture;
		friend class	BView;
		friend class	BFont;
		void			GetData(int32 *opCount, int32 *ptCount, uint32 **opList, BPoint **ptList);
		void			SetData(int32 opCount, int32 ptCount, uint32 *opList, BPoint *ptList);
		void			InitData();

		uint32			fState;
		uint32			fBuildingOp;
		void *			fPrivateData;
		uint32			reserved[4];
};

#endif
