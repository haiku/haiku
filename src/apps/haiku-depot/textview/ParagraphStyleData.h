/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_STYLE_DATA_H
#define PARAGRAPH_STYLE_DATA_H

#include <Referenceable.h>


enum Alignment {
	ALIGN_LEFT		= 0,
	ALIGN_CENTER	= 1,
	ALIGN_RIGHT		= 2,
};

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

			ParagraphStyleData*	SetAlignment(::Alignment alignment);
	inline	::Alignment			Alignment() const
									{ return fAlignment; }

			ParagraphStyleData*	SetJustify(bool justify);
	inline	bool				Justify() const
									{ return fJustify; }

			ParagraphStyleData*	SetFirstLineInset(float inset);
	inline	float				FirstLineInset() const
									{ return fFirstLineInset; }

			ParagraphStyleData*	SetLineInset(float inset);
	inline	float				LineInset() const
									{ return fLineInset; }

			ParagraphStyleData*	SetSpacingTop(float spacing);
	inline	float				SpacingTop() const
									{ return fSpacingTop; }

			ParagraphStyleData*	SetSpacingBottom(float spacing);
	inline	float				SpacingBottom() const
									{ return fSpacingBottom; }

private:
			ParagraphStyleData&	operator=(const ParagraphStyleData& other);

private:
			::Alignment			fAlignment;
			bool				fJustify;

			float				fFirstLineInset;
			float				fLineInset;

			float				fSpacingTop;
			float				fSpacingBottom;
};


typedef BReference<ParagraphStyleData>	ParagraphDataRef;


#endif // PARAGRAPH_STYLE_DATA_H
