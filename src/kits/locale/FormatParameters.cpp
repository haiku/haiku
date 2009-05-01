#include <FormatParameters.h>

// defaults
static const format_alignment kDefaultAlignment = B_ALIGN_FORMAT_RIGHT;
static const size_t kDefaultFormatWidth = 1;

// flags
enum {
	ALIGNMENT_SET	= 0x01,
	WIDTH_SET		= 0x02,
};

// constructor
BFormatParameters::BFormatParameters(const BFormatParameters *parent)
	: fParent(parent),
	  fFlags(0)
{
}

// copy constructor
BFormatParameters::BFormatParameters(const BFormatParameters &other)
	: fParent(other.fParent),
	  fAlignment(other.fAlignment),
	  fWidth(other.fWidth),
	  fFlags(other.fFlags)
{
}

// destructor
BFormatParameters::~BFormatParameters()
{
}

// SetAlignment
void
BFormatParameters::SetAlignment(format_alignment alignment)
{
	fAlignment = alignment;
	fFlags |= ALIGNMENT_SET;
}

// Alignment
format_alignment
BFormatParameters::Alignment() const
{
	if (fFlags & ALIGNMENT_SET)
		return fAlignment;
	if (fParent)
		return fParent->Alignment();
	return kDefaultAlignment;
}

// SetFormatWidth
void
BFormatParameters::SetFormatWidth(size_t width)
{
	fWidth = width;
	fFlags |= WIDTH_SET;
}

// FormatWidth
size_t
BFormatParameters::FormatWidth() const
{
	if (fFlags & WIDTH_SET)
		return fWidth;
	if (fParent)
		return fParent->FormatWidth();
	return kDefaultFormatWidth;
}

// SetParentParameters
void
BFormatParameters::SetParentParameters(const BFormatParameters *parent)
{
	fParent = parent;
}

// ParentParameters
const BFormatParameters *
BFormatParameters::ParentParameters() const
{
	return fParent;
}

// =
BFormatParameters &
BFormatParameters::operator=(const BFormatParameters &other)
{
	fParent = other.fParent;
	fAlignment = other.fAlignment;
	fWidth = other.fWidth;
	fFlags = other.fFlags;
	return *this;
}

