/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_ALIGNMENT_H
#define	_ALIGNMENT_H

#include <InterfaceDefs.h>

class BAlignment {
public:
			alignment			horizontal;
			vertical_alignment	vertical;

	inline						BAlignment();
	inline						BAlignment(const BAlignment& other);
	inline						BAlignment(alignment horizontal,
									vertical_alignment vertical);

	inline	alignment			Horizontal() const;
	inline	vertical_alignment	Vertical() const;

			float				RelativeHorizontal() const;
			float				RelativeVertical() const;

	inline	void				SetHorizontal(alignment horizontal);
	inline	void				SetVertical(vertical_alignment vertical);

	inline	bool				IsHorizontalSet() const;
	inline	bool				IsVerticalSet() const;

	inline	bool				operator==(const BAlignment& other) const;
	inline	bool				operator!=(const BAlignment& other) const;

	inline	BAlignment&			operator=(const BAlignment& other);
};


// constructor
inline
BAlignment::BAlignment()
	: horizontal(B_ALIGN_HORIZONTAL_UNSET),
	  vertical(B_ALIGN_VERTICAL_UNSET)
{
}

// copy constructor
inline
BAlignment::BAlignment(const BAlignment& other)
	: horizontal(other.horizontal),
	  vertical(other.vertical)
{
}

// constructor
inline
BAlignment::BAlignment(alignment horizontal, vertical_alignment vertical)
	: horizontal(horizontal),
	  vertical(vertical)
{
}

// Horizontal
inline alignment
BAlignment::Horizontal() const
{
	return horizontal;
}

// Vertical
inline vertical_alignment
BAlignment::Vertical() const
{
	return vertical;
}

// SetHorizontal
inline void
BAlignment::SetHorizontal(alignment horizontal)
{
	this->horizontal = horizontal;
}

// SetVertical
inline void
BAlignment::SetVertical(vertical_alignment vertical)
{
	this->vertical = vertical;
}

// IsHorizontalSet
inline bool
BAlignment::IsHorizontalSet() const
{
	return (horizontal != B_ALIGN_HORIZONTAL_UNSET);
}

// IsVerticalSet
inline bool
BAlignment::IsVerticalSet() const
{
	return (vertical != B_ALIGN_VERTICAL_UNSET);
}

// ==
inline bool
BAlignment::operator==(const BAlignment& other) const
{
	return (horizontal == other.horizontal && vertical == other.vertical);
}

// !=
inline bool
BAlignment::operator!=(const BAlignment& other) const
{
	return !(*this == other);
}

// =
inline BAlignment&
BAlignment::operator=(const BAlignment& other)
{
	horizontal = other.horizontal;
	vertical = other.vertical;
	return *this;
}

#endif	// _ALIGNMENT_H
