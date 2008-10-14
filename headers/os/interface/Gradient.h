/*
 * Copyright 2006-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski <harakash@gmail.com>
 */

#ifndef GRADIENT_H
#define GRADIENT_H

#include <Archivable.h>
#include <GraphicsDefs.h>
#include <List.h>


class BMessage;
class BRect;


enum gradient_type {
	B_GRADIENT_LINEAR = 0,
	B_GRADIENT_RADIAL,
	B_GRADIENT_RADIAL_FOCUS,
	B_GRADIENT_DIAMOND,
	B_GRADIENT_CONIC,
	B_GRADIENT_NONE
};


struct color_step {
	color_step(const rgb_color c, float o);
	color_step(uint8 r, uint8 g, uint8 b, uint8 a, float o);
	color_step(const color_step& other);
	color_step();
	
	bool operator!=(const color_step& other) const;
	
	rgb_color		color;
	float			offset;
};


class BGradient : public BArchivable {
public:
						BGradient();
						BGradient(BMessage* archive);
	virtual				~BGradient();
	
	status_t			Archive(BMessage* into, bool deep = true) const;
	
	BGradient&			operator=(const BGradient& other);
	
	bool				operator==(const BGradient& other) const;
	bool				operator!=(const BGradient& other) const;
	bool				ColorStepsAreEqual(const BGradient& other) const;
	
	void				SetColors(const BGradient& other);
	
	int32				AddColor(const rgb_color& color, float offset);
	bool				AddColor(const color_step& color, int32 index);
	
	bool				RemoveColor(int32 index);
	
	bool				SetColor(int32 index, const color_step& step);
	bool				SetColor(int32 index, const rgb_color& color);
	bool				SetOffset(int32 index, float offset);
	
	int32				CountColors() const;
	color_step*			ColorAt(int32 index) const;
	color_step*			ColorAtFast(int32 index) const;
	color_step*			Colors() const;
	void				SortColorStepsByOffset();
	
	gradient_type		Type() const
							{ return fType; }
	
	void				MakeEmpty();
	
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

	BList				fColors;
	gradient_type		fType;
};

#endif // GRADIENT_H
