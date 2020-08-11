/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRADIENT_H
#define _GRADIENT_H


#include <Archivable.h>
#include <GraphicsDefs.h>
#include <List.h>


class BDataIO;
class BMessage;
class BRect;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku. In particular,
// the offsets are currently specified on [0..255], but may be changed
// to the interval [0..1]. This class also does not have any FBC padding,
// So your software will definitely break when this class gets new
// virtuals. And the object size may change too...


class BGradient : public BArchivable {
public:
	enum Type {
		TYPE_LINEAR = 0,
		TYPE_RADIAL,
		TYPE_RADIAL_FOCUS,
		TYPE_DIAMOND,
		TYPE_CONIC,
		TYPE_NONE
	};

	struct ColorStop {
		ColorStop(const rgb_color c, float o);
		ColorStop(uint8 r, uint8 g, uint8 b, uint8 a, float o);
		ColorStop(const ColorStop& other);
		ColorStop();

		bool operator!=(const ColorStop& other) const;

		rgb_color		color;
		float			offset;
	};

public:
								BGradient();
								BGradient(const BGradient& other);
								BGradient(BMessage* archive);
	virtual						~BGradient();

			status_t			Archive(BMessage* into,
									bool deep = true) const;

			BGradient&			operator=(const BGradient& other);

			bool				operator==(const BGradient& other) const;
			bool				operator!=(const BGradient& other) const;
			bool				ColorStopsAreEqual(
									const BGradient& other) const;

			void				SetColorStops(const BGradient& other);

			int32				AddColor(const rgb_color& color,
									float offset);
			bool				AddColorStop(const ColorStop& colorStop,
									int32 index);

			bool				RemoveColor(int32 index);

			bool				SetColorStop(int32 index,
									const ColorStop& colorStop);
			bool				SetColor(int32 index, const rgb_color& color);
			bool				SetOffset(int32 index, float offset);

			int32				CountColorStops() const;
			ColorStop*			ColorStopAt(int32 index) const;
			ColorStop*			ColorStopAtFast(int32 index) const;
			ColorStop*			ColorStops() const;
			void				SortColorStopsByOffset();

			Type				GetType() const
									{ return fType; }

			void				MakeEmpty();

			status_t			Flatten(BDataIO* stream) const;
	static	status_t			Unflatten(BGradient *&output, BDataIO* stream);

private:
	friend class BGradientLinear;
	friend class BGradientRadial;
	friend class BGradientRadialFocus;
	friend class BGradientDiamond;
	friend class BGradientConic;

			union {
				struct {
					float x1, y1, x2, y2;
				} linear;
				struct {
					float cx, cy, radius;
				} radial;
				struct {
					float cx, cy, fx, fy, radius;
				} radial_focus;
				struct {
					float cx, cy;
				} diamond;
				struct {
					float cx, cy, angle;
				} conic;
			} fData;

			BList				fColorStops;
			Type				fType;
};

#endif // _GRADIENT_H
