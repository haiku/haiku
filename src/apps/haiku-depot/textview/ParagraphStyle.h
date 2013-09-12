/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PARAGRAPH_STYLE_H
#define PARAGRAPH_STYLE_H

#include "ParagraphStyleData.h"


class ParagraphStyle {
public:
								ParagraphStyle();
								ParagraphStyle(const ParagraphStyle& other);

			ParagraphStyle&		operator=(const ParagraphStyle& other);
			bool				operator==(const ParagraphStyle& other) const;
			bool				operator!=(const ParagraphStyle& other) const;

			bool				SetAlignment(::Alignment alignment);
			::Alignment			Alignment() const;

			bool				SetJustify(bool justify);
			bool				Justify() const;

			bool				SetFirstLineInset(float inset);
			float				FirstLineInset() const;

			bool				SetLineInset(float inset);
			float				LineInset() const;

			bool				SetLineSpacing(float spacing);
			float				LineSpacing() const;

			bool				SetSpacingTop(float spacing);
			float				SpacingTop() const;

			bool				SetSpacingBottom(float spacing);
			float				SpacingBottom() const;

			bool				SetBullet(const ::Bullet& bullet);
			const ::Bullet&		Bullet() const;

private:
			ParagraphStyleDataRef fStyleData;
};


#endif // PARAGRAPH_STYLE_H
