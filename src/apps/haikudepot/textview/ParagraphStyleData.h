/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_STYLE_DATA_H
#define PARAGRAPH_STYLE_DATA_H

#include "Bullet.h"


enum Alignment {
	ALIGN_LEFT		= 0,
	ALIGN_CENTER	= 1,
	ALIGN_RIGHT		= 2,
};


class ParagraphStyleData;
typedef BReference<ParagraphStyleData>	ParagraphStyleDataRef;


// You cannot modify a ParagraphStyleData object once it has been
// created.
class ParagraphStyleData : public BReferenceable {
public:
								ParagraphStyleData();
								ParagraphStyleData(
									const ParagraphStyleData& other);

			bool				operator==(
									const ParagraphStyleData& other) const;
			bool				operator!=(
									const ParagraphStyleData& other) const;

			ParagraphStyleDataRef SetAlignment(::Alignment alignment);
	inline	::Alignment			Alignment() const
									{ return fAlignment; }

			ParagraphStyleDataRef SetJustify(bool justify);
	inline	bool				Justify() const
									{ return fJustify; }

			ParagraphStyleDataRef SetFirstLineInset(float inset);
	inline	float				FirstLineInset() const
									{ return fFirstLineInset; }

			ParagraphStyleDataRef SetLineInset(float inset);
	inline	float				LineInset() const
									{ return fLineInset; }

			ParagraphStyleDataRef SetLineSpacing(float spacing);
	inline	float				LineSpacing() const
									{ return fLineSpacing; }

			ParagraphStyleDataRef SetSpacingTop(float spacing);
	inline	float				SpacingTop() const
									{ return fSpacingTop; }

			ParagraphStyleDataRef SetSpacingBottom(float spacing);
	inline	float				SpacingBottom() const
									{ return fSpacingBottom; }

			ParagraphStyleDataRef SetBullet(const ::Bullet& bullet);
	inline	const ::Bullet&		Bullet() const
									{ return fBullet; }
private:
			ParagraphStyleData&	operator=(const ParagraphStyleData& other);

private:
			::Alignment			fAlignment;
			bool				fJustify;

			float				fFirstLineInset;
			float				fLineInset;
			float				fLineSpacing;

			float				fSpacingTop;
			float				fSpacingBottom;

			::Bullet			fBullet;
};


#endif // PARAGRAPH_STYLE_DATA_H
