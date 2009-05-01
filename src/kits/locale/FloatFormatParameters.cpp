#include <FloatFormatParameters.h>

// defaults
static const size_t kDefaultMinimalFractionDigits = 0;
static const size_t kDefaultMaximalFractionDigits = 6;
static const bool kDefaultUseUpperCase = false;
static const float_format_type kDefaultFloatFormatType = B_AUTO_FLOAT_FORMAT;
static const bool kDefaultAlwaysUseFractionSeparator = false;
static const bool kDefaultKeepTrailingFractionZeros = false;


// flags
enum {
	MINIMAL_FRACTION_DIGITS_SET			= 0x01,
	MAXIMAL_FRACTION_DIGITS_SET			= 0x02,
	USE_UPPER_CASE_SET					= 0x04,
	FLOAT_FORMAT_TYPE_SET				= 0x08,
	ALWAYS_USE_FRACTION_SEPARATOR_SET	= 0x10,
	KEEP_TRAILING_FRACTION_ZEROS_SET	= 0x20,
};

// constructor
BFloatFormatParameters::BFloatFormatParameters(
	const BFloatFormatParameters *parent)
	: BNumberFormatParameters(parent),
	  fParent(parent),
	  fFlags(0)
{
}

// copy constructor
BFloatFormatParameters::BFloatFormatParameters(
	const BFloatFormatParameters &other)
	: BNumberFormatParameters(other),
	  fParent(other.fParent),
	  fMinimalFractionDigits(other.fMinimalFractionDigits),
	  fMaximalFractionDigits(other.fMaximalFractionDigits),
	  fUseUpperCase(other.fUseUpperCase),
	  fFloatFormatType(other.fFloatFormatType),
	  fAlwaysUseFractionSeparator(other.fAlwaysUseFractionSeparator),
	  fKeepTrailingFractionZeros(other.fKeepTrailingFractionZeros),
	  fFlags(other.fFlags)
{
}

// destructor
BFloatFormatParameters::~BFloatFormatParameters()
{
}

// SetMinimalFractionDigits
void
BFloatFormatParameters::SetMinimalFractionDigits(size_t minFractionDigits)
{
	fMinimalFractionDigits = minFractionDigits;
	fFlags |= MINIMAL_FRACTION_DIGITS_SET;
}

// MinimalFractionDigits
size_t
BFloatFormatParameters::MinimalFractionDigits() const
{
	if (fFlags & MINIMAL_FRACTION_DIGITS_SET)
		return fMinimalFractionDigits;
	if (fParent)
		return fParent->MinimalFractionDigits();
	return kDefaultMinimalFractionDigits;
}

// SetMaximalFractionDigits
void
BFloatFormatParameters::SetMaximalFractionDigits(size_t maxFractionDigits)
{
	fMaximalFractionDigits = maxFractionDigits;
	fFlags |= MAXIMAL_FRACTION_DIGITS_SET;
}

// MaximalFractionDigits
size_t
BFloatFormatParameters::MaximalFractionDigits() const
{
	if (fFlags & MAXIMAL_FRACTION_DIGITS_SET)
		return fMaximalFractionDigits;
	if (fParent)
		return fParent->MaximalFractionDigits();
	return kDefaultMaximalFractionDigits;
}

// SetUseUpperCase
void
BFloatFormatParameters::SetUseUpperCase(bool useCapitals)
{
	fUseUpperCase = useCapitals;
	fFlags |= USE_UPPER_CASE_SET;
}

// UseUpperCase
bool
BFloatFormatParameters::UseUpperCase() const
{
	if (fFlags & USE_UPPER_CASE_SET)
		return fUseUpperCase;
	if (fParent)
		return fParent->UseUpperCase();
	return kDefaultUseUpperCase;
}

// SetFloatFormatType
void
BFloatFormatParameters::SetFloatFormatType(float_format_type type)
{
	fFloatFormatType = type;
	fFlags |= FLOAT_FORMAT_TYPE_SET;
}

// FloatFormatType
float_format_type
BFloatFormatParameters::FloatFormatType() const
{
	if (fFlags & FLOAT_FORMAT_TYPE_SET)
		return fFloatFormatType;
	if (fParent)
		return fParent->FloatFormatType();
	return kDefaultFloatFormatType;
}

// SetAlwaysUseFractionSeparator
void
BFloatFormatParameters::SetAlwaysUseFractionSeparator(
	bool alwaysUseFractionSeparator)
{
	fAlwaysUseFractionSeparator = alwaysUseFractionSeparator;
	fFlags |= ALWAYS_USE_FRACTION_SEPARATOR_SET;
}

// AlwaysUseFractionSeparator
bool
BFloatFormatParameters::AlwaysUseFractionSeparator() const
{
	if (fFlags & ALWAYS_USE_FRACTION_SEPARATOR_SET)
		return fAlwaysUseFractionSeparator;
	if (fParent)
		return fParent->AlwaysUseFractionSeparator();
	return kDefaultAlwaysUseFractionSeparator;
}

// SetKeepTrailingFractionZeros
void
BFloatFormatParameters::SetKeepTrailingFractionZeros(
	bool keepTrailingFractionZeros)
{
	fKeepTrailingFractionZeros = keepTrailingFractionZeros;
	fFlags |= KEEP_TRAILING_FRACTION_ZEROS_SET;
}

// KeepTrailingFractionZeros
bool
BFloatFormatParameters::KeepTrailingFractionZeros() const
{
	if (fFlags & KEEP_TRAILING_FRACTION_ZEROS_SET)
		return fKeepTrailingFractionZeros;
	if (fParent)
		return fParent->KeepTrailingFractionZeros();
	return kDefaultKeepTrailingFractionZeros;
}

// SetParentFloatParameters
void
BFloatFormatParameters::SetParentFloatParameters(
	const BFloatFormatParameters *parent)
{
	fParent = parent;
	SetParentNumberParameters(parent);
}

// ParentFloatParameters
const BFloatFormatParameters *
BFloatFormatParameters::ParentFloatParameters() const
{
	return fParent;
}

// =
BFloatFormatParameters &
BFloatFormatParameters::operator=(const BFloatFormatParameters &other)
{
	BNumberFormatParameters::operator=(other);
	fParent = other.fParent;
	fMinimalFractionDigits = other.fMinimalFractionDigits;
	fMaximalFractionDigits = other.fMaximalFractionDigits;
	fUseUpperCase = other.fUseUpperCase;
	fFloatFormatType = other.fFloatFormatType;
	fAlwaysUseFractionSeparator = other.fAlwaysUseFractionSeparator;
	fKeepTrailingFractionZeros = other.fKeepTrailingFractionZeros;
	fFlags = other.fFlags;
	return *this;
}

