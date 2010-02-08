#include <algorithm>
#include <float.h>
#include <math.h>
#include <new>
#include <stdlib.h>

#include <GenericNumberFormat.h>
#include <String.h>
#include <UnicodeChar.h>

#if __GNUC__ > 2
using std::max;
using std::min;
using std::nothrow;
#endif

// constants (more below the helper classes)

static const int kMaxIntDigitCount = 20;	// int64: 19 + sign, uint64: 20
static const int kMaxFloatDigitCount = DBL_DIG + 2;
	// double: mantissa precision + 2


// Symbol

// constructor
BGenericNumberFormat::Symbol::Symbol(const char *symbol)
	: symbol(NULL),
	  length(0),
	  char_count(0)
{
	SetTo(symbol);
}

// destructor
BGenericNumberFormat::Symbol::~Symbol()
{
	Unset();
}

// SetTo
status_t
BGenericNumberFormat::Symbol::SetTo(const char *symbol)
{
	// unset old
	if (this->symbol) {
		free(this->symbol);
		length = 0;
		char_count = 0;
	}
	// set new
	if (symbol) {
		this->symbol = strdup(symbol);
		if (!this->symbol)
			return B_NO_MEMORY;
		length = strlen(this->symbol);
		char_count = BUnicodeChar::UTF8StringLength(this->symbol);
	}
	return B_OK;
}


// SpecialNumberSymbols
struct BGenericNumberFormat::SpecialNumberSymbols {
	const Symbol	*nan;
	const Symbol	*infinity;
	const Symbol	*negative_infinity;
};


// GroupingInfo
class BGenericNumberFormat::GroupingInfo {
	public:
		GroupingInfo()
			: fSeparators(NULL),
			  fSeparatorCount(0),
			  fSizes(NULL),
			  fSizeCount(0),
			  fSumSizes(NULL),
			  fSumSeparators(NULL)
		{
		}

		GroupingInfo(const char **separators, int32 separatorCount,
					 const size_t *sizes, int32 sizeCount)
			: fSeparators(NULL),
			  fSeparatorCount(0),
			  fSizes(NULL),
			  fSizeCount(0),
			  fSumSizes(NULL),
			  fSumSeparators(NULL)
		{
			SetTo(separators, separatorCount, sizes, sizeCount);
		}

		~GroupingInfo()
		{
			Unset();
		}

		status_t SetTo(const char **separators, int32 separatorCount,
					   const size_t *sizes, int32 sizeCount)
		{
			// unset old
			Unset();
			// set new
			if ((!separators && separatorCount <= 0)
				|| (!sizes && sizeCount <= 0))
				return B_OK;
			// allocate arrays
			fSeparators = new(nothrow) Symbol[separatorCount];
			fSizes = new(nothrow) int32[sizeCount];
			fSumSizes = new(nothrow) int32[sizeCount];
			fSumSeparators = new(nothrow) Symbol*[separatorCount];
			if (!fSeparators || !fSizes || !fSumSizes || !fSumSeparators) {
				Unset();
				return B_NO_MEMORY;
			}
			fSeparatorCount = separatorCount;
			fSizeCount = sizeCount;
			// separators
			for (int i = 0; i < separatorCount; i++) {
				status_t error = fSeparators[i].SetTo(separators[i]);
				if (error != B_OK) {
					Unset();
					return error;
				}
			}
			// sizes and sum arrays
			int32 sumSize = -1;
			for (int32 i = 0; i < sizeCount; i++) {
				fSizes[i] = (int32)sizes[i];
				sumSize += fSizes[i];
				fSumSizes[i] = sumSize;
				fSumSeparators[i] = &fSeparators[min(i, fSeparatorCount)];
			}
			return B_OK;
		}

		void Unset()
		{
			if (fSeparators) {
				delete[] fSeparators;
				fSeparators = NULL;
			}
			fSeparatorCount = 0;
			if (fSizes) {
				delete[] fSizes;
				fSizes = NULL;
			}
			fSizeCount = 0;
			if (fSumSizes) {
				delete[] fSumSizes;
				fSumSizes = NULL;
			}
			if (fSumSeparators) {
				delete[] fSumSeparators;
				fSumSeparators = NULL;
			}
		}

		const Symbol *SeparatorForDigit(int32 position) const
		{
			for (int i = fSizeCount - 1; i >= 0; i--) {
				if (fSumSizes[i] <= position) {
					if (fSumSizes[i] == position
						|| (i == fSizeCount - 1
							&& (position - fSumSizes[i]) % fSizes[i] == 0)) {
						return fSumSeparators[i];
					}
					return NULL;
				}
			}
			return NULL;
		}

	private:
		Symbol	*fSeparators;
		int32	fSeparatorCount;
		int32	*fSizes;
		int32	fSizeCount;
		int32	*fSumSizes;
		Symbol	**fSumSeparators;
};


// SignSymbols
class BGenericNumberFormat::SignSymbols {
	public:
		SignSymbols()
			: fPlusPrefix(),
			  fMinusPrefix(),
			  fPadPlusPrefix(),
			  fNoForcePlusPrefix(),
			  fPlusSuffix(),
			  fMinusSuffix(),
			  fPadPlusSuffix(),
			  fNoForcePlusSuffix()
		{
		}

		SignSymbols(const char *plusPrefix, const char *minusPrefix,
			const char *padPlusPrefix, const char *noForcePlusPrefix,
			const char *plusSuffix, const char *minusSuffix,
			const char *padPlusSuffix, const char *noForcePlusSuffix)
			: fPlusPrefix(plusPrefix),
			  fMinusPrefix(minusPrefix),
			  fPadPlusPrefix(padPlusPrefix),
			  fNoForcePlusPrefix(noForcePlusPrefix),
			  fPlusSuffix(plusSuffix),
			  fMinusSuffix(minusSuffix),
			  fPadPlusSuffix(padPlusSuffix),
			  fNoForcePlusSuffix(noForcePlusSuffix)
		{
		}

		~SignSymbols()
		{
		}

		status_t SetTo(const char *plusPrefix, const char *minusPrefix,
			const char *padPlusPrefix, const char *noForcePlusPrefix,
			const char *plusSuffix, const char *minusSuffix,
			const char *padPlusSuffix, const char *noForcePlusSuffix)
		{
			status_t error = B_OK;
			if (error == B_OK)
				error = fPlusPrefix.SetTo(plusPrefix);
			if (error == B_OK)
				error = fMinusPrefix.SetTo(minusPrefix);
			if (error == B_OK)
				error = fPadPlusPrefix.SetTo(noForcePlusPrefix);
			if (error == B_OK)
				error = fNoForcePlusPrefix.SetTo(noForcePlusPrefix);
			if (error == B_OK)
				error = fPlusSuffix.SetTo(plusSuffix);
			if (error == B_OK)
				error = fMinusSuffix.SetTo(minusSuffix);
			if (error == B_OK)
				error = fPadPlusSuffix.SetTo(noForcePlusSuffix);
			if (error == B_OK)
				error = fNoForcePlusSuffix.SetTo(noForcePlusSuffix);
			if (error != B_OK)
				Unset();
			return error;
		}

		void Unset()
		{
			fPlusPrefix.Unset();
			fMinusPrefix.Unset();
			fNoForcePlusPrefix.Unset();
			fPadPlusPrefix.Unset();
			fPlusSuffix.Unset();
			fMinusSuffix.Unset();
			fNoForcePlusSuffix.Unset();
			fPadPlusSuffix.Unset();
		}

		const Symbol *PlusPrefix() const
		{
			return &fPlusPrefix;
		}

		const Symbol *MinusPrefix() const
		{
			return &fMinusPrefix;
		}

		const Symbol *PadPlusPrefix() const
		{
			return &fPadPlusPrefix;
		}

		const Symbol *NoForcePlusPrefix() const
		{
			return &fNoForcePlusPrefix;
		}

		const Symbol *PlusSuffix() const
		{
			return &fPlusSuffix;
		}

		const Symbol *MinusSuffix() const
		{
			return &fMinusSuffix;
		}

		const Symbol *PadPlusSuffix() const
		{
			return &fPadPlusSuffix;
		}

		const Symbol *NoForcePlusSuffix() const
		{
			return &fNoForcePlusSuffix;
		}

	private:
		Symbol	fPlusPrefix;
		Symbol	fMinusPrefix;
		Symbol	fPadPlusPrefix;
		Symbol	fNoForcePlusPrefix;
		Symbol	fPlusSuffix;
		Symbol	fMinusSuffix;
		Symbol	fPadPlusSuffix;
		Symbol	fNoForcePlusSuffix;
};


// BufferWriter
class BGenericNumberFormat::BufferWriter {
	public:
		BufferWriter(char *buffer = NULL, int32 bufferSize = 0)
		{
			SetTo(buffer, bufferSize);
		}

		void SetTo(char *buffer = NULL, int32 bufferSize = 0)
		{
			fBuffer = buffer;
			fBufferSize = bufferSize;
			fPosition = 0;
			fCharCount = 0;
			fDryRun = (!fBuffer || (fBufferSize == 0));
			if (!fDryRun)
				fBuffer[0] = '\0';
		}

		int32 StringLength() const
		{
			return fPosition;
		}

		int32 CharCount() const
		{
			return fCharCount;
		}

		bool IsOverflow() const
		{
			return (fPosition >= fBufferSize);
		}

		void Append(const char *bytes, size_t length, size_t charCount)
		{
			int32 newPosition = fPosition + length;
			fDryRun |= (newPosition >= fBufferSize);
			if (!fDryRun && length > 0) {
				memcpy(fBuffer + fPosition, bytes, length);
				fBuffer[newPosition] = '\0';
			}
			fPosition = newPosition;
			fCharCount += charCount;
		}

		void Append(const Symbol &symbol)
		{
			Append(symbol.symbol, symbol.length, symbol.char_count);
		}

		void Append(const Symbol *symbol)
		{
			if (symbol)
				Append(*symbol);
		}

		void Append(char c, int32 count)	// ASCII 128 chars only!
		{
			if (count <= 0)
				return;
			int32 newPosition = fPosition + count;
			fDryRun |= (newPosition >= fBufferSize);
			if (!fDryRun && count > 0) {
				memset(fBuffer + fPosition, c, count);
				fBuffer[newPosition] = '\0';
			}
			fPosition = newPosition;
			fCharCount += count;
		}

		void Append(const Symbol &symbol, int32 count)
		{
			if (count <= 0)
				return;
			int32 bytes = count * symbol.length;
			int32 newPosition = fPosition + bytes;
			fDryRun |= (newPosition >= fBufferSize);
			if (!fDryRun && count > 0) {
				for (int i = 0; i < count * symbol.length; i++)
					fBuffer[i] = symbol.symbol[i % symbol.length];
				fBuffer[newPosition] = '\0';
			}
			fPosition = newPosition;
			fCharCount += count * symbol.char_count;
		}

		void Append(const Symbol *symbol, int32 count)
		{
			if (symbol)
				Append(*symbol, count);
		}

	private:
		char	*fBuffer;
		int32	fBufferSize;
		int32	fPosition;
		int32	fCharCount;
		bool	fDryRun;
};


// constants

// digit symbols
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultDigitSymbols[] = {
		"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
	};

// decimal separator symbol
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultFractionSeparator = ".";

// grouping separator symbols
static const char *kDefaultGroupingSeparators[] = { "," };
static const int32 kDefaultGroupingSeparatorCount
	= sizeof(kDefaultGroupingSeparators) / sizeof(const char*);
static const char *kNoGroupingSeparators[] = { NULL };	// to please mwcc
static const int32 kNoGroupingSeparatorCount = 0;

// grouping sizes
static const size_t kDefaultGroupingSizes[] = { 3 };
static const int32 kDefaultGroupingSizeCount
	= sizeof(kDefaultGroupingSizes) / sizeof(size_t);
static const size_t kNoGroupingSizes[] = { 0 };			// to please mwcc
static const int32 kNoGroupingSizeCount = 0;

// grouping info
const BGenericNumberFormat::GroupingInfo
	BGenericNumberFormat::kDefaultGroupingInfo(
		kDefaultGroupingSeparators, kDefaultGroupingSeparatorCount,
		kDefaultGroupingSizes, kDefaultGroupingSizeCount
	);
const BGenericNumberFormat::GroupingInfo
	BGenericNumberFormat::kNoGroupingInfo(
		kNoGroupingSeparators, kNoGroupingSeparatorCount,
		kNoGroupingSizes, kNoGroupingSizeCount
	);

// exponent symbol
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultExponentSymbol = "e";
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultUpperCaseExponentSymbol = "E";

// NaN symbol
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultNaNSymbol = "NaN";
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultUpperCaseNaNSymbol = "NaN";

// infinity symbol
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultInfinitySymbol = "infinity";
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultUpperCaseInfinitySymbol = "INFINITY";

// negative infinity symbol
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultNegativeInfinitySymbol = "-infinity";
const BGenericNumberFormat::Symbol
	BGenericNumberFormat::kDefaultUpperCaseNegativeInfinitySymbol = "-INFINITY";

// sign symbols
const BGenericNumberFormat::SignSymbols
	BGenericNumberFormat::kDefaultSignSymbols(
		"+", "-", " ", "",	// prefixes
		"", "", "", ""		// suffixes
	);

// mantissa sign symbols
const BGenericNumberFormat::SignSymbols
	BGenericNumberFormat::kDefaultMantissaSignSymbols(
		"", "", "", "",	// prefixes
		"", "", "", ""		// suffixes
	);

// exponent sign symbols
const BGenericNumberFormat::SignSymbols
	BGenericNumberFormat::kDefaultExponentSignSymbols(
		"+", "-", " ", "",	// prefixes
		"", "", "", ""		// suffixes
	);


// Integer
class BGenericNumberFormat::Integer {
	public:
		Integer(int64 number)
			: fDigitCount(0),
			  fNegative(number < 0)
		{
			if (fNegative)
				Init(0ULL - (uint64)number);
			else
				Init(number);
		}

		Integer(uint64 number)
			: fDigitCount(0),
			  fNegative(false)
		{
			Init(number);
		}

		int DigitCount() const
		{
			return fDigitCount;
		}

		bool IsNegative() const
		{
			return fNegative;
		}

		char *ToString(char *str) const
		{
			if (fDigitCount == 0) {
				str[0] = '0';
				str[1] = '\0';
			} else if (fNegative) {
				str[0] = '-';
				for (int i = 0; i < fDigitCount; i++)
					str[i + 1] = '0' + fDigits[fDigitCount - i - 1];
				str[fDigitCount + 1] = '\0';
			} else {
				for (int i = 0; i < fDigitCount; i++)
					str[i] = '0' + fDigits[fDigitCount - i - 1];
				str[fDigitCount] = '\0';
			}
			return str;
		}

		void Format(BufferWriter &writer, const Symbol *digitSymbols,
			const SignSymbols *signSymbols,
			number_format_sign_policy signPolicy,
			const GroupingInfo *groupingInfo, int32 minDigits)  const
		{
			const Symbol *suffix = NULL;
			// write sign prefix
			if (fNegative) {
				writer.Append(signSymbols->MinusPrefix());
				suffix = signSymbols->MinusSuffix();
			} else {
				switch (signPolicy) {
					case B_USE_NEGATIVE_SIGN_ONLY:
						writer.Append(signSymbols->NoForcePlusPrefix());
						suffix = signSymbols->NoForcePlusSuffix();
						break;
					case B_USE_SPACE_FOR_POSITIVE_SIGN:
						writer.Append(signSymbols->PadPlusPrefix());
						suffix = signSymbols->PadPlusSuffix();
						break;
					case B_USE_POSITIVE_SIGN:
						writer.Append(signSymbols->PlusPrefix());
						suffix = signSymbols->PlusSuffix();
						break;
				}
			}
			// the digits
			if (fDigitCount == 0 && minDigits < 1) {
				// special case for zero and less the one minimal digit
				writer.Append(digitSymbols[0]);
			} else {
				// not zero or at least one minimal digit
				if (groupingInfo) {
					// use grouping
					// pad with zeros up to minDigits
					int32 digitCount = max(fDigitCount, minDigits);
					for (int i = minDigits - 1; i >= fDigitCount; i--) {
						if (i != digitCount - 1)
							writer.Append(groupingInfo->SeparatorForDigit(i));
						writer.Append(digitSymbols[0]);
					}
					// write digits
					for (int i = fDigitCount - 1; i >= 0; i--) {
						if (i != digitCount - 1)
							writer.Append(groupingInfo->SeparatorForDigit(i));
						writer.Append(digitSymbols[fDigits[i]]);
					}
				} else {
					// no grouping
					// pad with zeros up to minDigits
					if (fDigitCount < minDigits)
						writer.Append(digitSymbols, minDigits - fDigitCount);
					// write digits
					for (int i = fDigitCount - 1; i >= 0; i--)
						writer.Append(digitSymbols[fDigits[i]]);
				}
			}
			// append suffix
			writer.Append(suffix);
		}

	private:
		void Init(uint64 number)
		{
			fDigitCount = 0;
			while (number) {
				fDigits[fDigitCount] = number % 10;
				number /= 10;
				fDigitCount++;
			}
		}

	private:
		uchar	fDigits[kMaxIntDigitCount];
		int32	fDigitCount;
		bool	fNegative;
};


// Float
class BGenericNumberFormat::Float {
	public:
		Float(double number)
			: fNegative(signbit(number)),
			  fClass(fpclassify(number)),
			  fExponent(0),
			  fDigitCount(0)
		{
			// filter special cases
			switch (fClass) {
				case FP_NAN:
				case FP_INFINITE:
					return;
				case FP_ZERO:
					fDigits[0] = 0;
					fDigitCount = 1;
					return;
				case FP_NORMAL:
				case FP_SUBNORMAL:
					break;
			}
			if (fNegative)
				number = -number;
			// We start with an exponent great enough to make
			// number / 10^fExponent < 10. It may even be < 1 or 0.1.
			// We simply cut those digits later.
			fExponent = (int)ceil(log10(number));
			int shiftBy = kMaxFloatDigitCount - fExponent - 1;
			// We don't multiply with 10^shiftBy not in one go, since for
			// subnormal numbers 10^shiftBy will not be representable. Maybe
			// also for normal numbers close to the limit -- so don't risk
			// anything, for the time being. TODO: Optimize later.
			double mantissa = number * pow(10, shiftBy / 2);
			mantissa *= pow(10, shiftBy - shiftBy / 2);
			// get the mantissa's digits -- we drop trailing zeros
			int32 firstNonNull = -1;
			for (int i = 0; i < kMaxFloatDigitCount; i++) {
				char digit = (char)fmod(mantissa, 10);
				if (firstNonNull < 0 && digit > 0)
					firstNonNull = i;
				if (firstNonNull >= 0)
					fDigits[i - firstNonNull] = digit;
				mantissa /= 10;
			}
			if (firstNonNull >= 0)
				fDigitCount = kMaxFloatDigitCount - firstNonNull;
			else
				fDigitCount = 0;
			// drop leading zeros
			while (fDigitCount > 0 && fDigits[fDigitCount - 1] == 0) {
				fDigitCount--;
				fExponent--;
			}
			// due to rounding effects we may end up with zero: switch to its
			// canaonical representation then
			if (fDigitCount == 0) {
				fExponent = 0;
				fDigits[0] = 0;
				fDigitCount = 1;
			}
		}

		void Format(BufferWriter &writer, const Symbol *digitSymbols,
			const SpecialNumberSymbols *specialNumbers,
			const Symbol *fractionSeparator,
			const Symbol *exponentSymbol,
			const SignSymbols *signSymbols,
			const SignSymbols *mantissaSignSymbols,
			const SignSymbols *exponentSignSymbols,
			float_format_type formatType,
			number_format_sign_policy signPolicy,
			const GroupingInfo *groupingInfo,
			int32 minIntegerDigits, int32 minFractionDigits,
			int32 maxFractionDigits, bool forceFractionSeparator,
			bool keepTrailingFractionZeros) const
		{
			// deal with special numbers
			switch (fClass) {
				case FP_NAN:
					writer.Append(specialNumbers->nan);
					return;
				case FP_INFINITE:
					if (fNegative)
						writer.Append(specialNumbers->negative_infinity);
					else
						writer.Append(specialNumbers->infinity);
					return;
				case FP_ZERO:
				case FP_NORMAL:
				case FP_SUBNORMAL:
					break;
			}
			// format according to the specified format type
			bool scientific = false;
			switch (formatType) {
				case B_FIXED_POINT_FLOAT_FORMAT:
					break;
				case B_SCIENTIFIC_FLOAT_FORMAT:
					scientific = true;
					break;
				case B_AUTO_FLOAT_FORMAT:
					// the criterion printf() uses:
					scientific = (fExponent >= maxFractionDigits
								  || fExponent < -4);
					break;
			}
			// finally invoke the respective method that does the formatting
			if (scientific) {
				FormatScientific(writer, digitSymbols, fractionSeparator,
					exponentSymbol, signSymbols, mantissaSignSymbols,
					exponentSignSymbols, signPolicy, minIntegerDigits,
					minFractionDigits, maxFractionDigits,
					forceFractionSeparator, keepTrailingFractionZeros);
			} else {
				FormatFixedPoint(writer, digitSymbols, fractionSeparator,
					signSymbols, mantissaSignSymbols, signPolicy, groupingInfo,
					minIntegerDigits, minFractionDigits, maxFractionDigits,
					forceFractionSeparator, keepTrailingFractionZeros);
			}
		}

		void FormatScientific(BufferWriter &writer, const Symbol *digitSymbols,
			const Symbol *fractionSeparator,
			const Symbol *exponentSymbol,
			const SignSymbols *signSymbols,
			const SignSymbols *mantissaSignSymbols,
			const SignSymbols *exponentSignSymbols,
			number_format_sign_policy signPolicy,
			int32 minIntegerDigits, int32 minFractionDigits,
			int32 maxFractionDigits, bool forceFractionSeparator,
			bool keepTrailingFractionZeros) const
		{
			const Symbol *suffix = NULL;
			const Symbol *mantissaSuffix = NULL;
			// write sign prefix
			if (fNegative) {
				writer.Append(signSymbols->MinusPrefix());
				writer.Append(mantissaSignSymbols->MinusPrefix());
				suffix = signSymbols->MinusSuffix();
				mantissaSuffix = mantissaSignSymbols->MinusSuffix();
			} else {
				switch (signPolicy) {
					case B_USE_NEGATIVE_SIGN_ONLY:
						writer.Append(signSymbols->NoForcePlusPrefix());
						writer.Append(mantissaSignSymbols->NoForcePlusPrefix());
						suffix = signSymbols->NoForcePlusSuffix();
						mantissaSuffix
							= mantissaSignSymbols->NoForcePlusSuffix();
						break;
					case B_USE_SPACE_FOR_POSITIVE_SIGN:
						writer.Append(signSymbols->PadPlusPrefix());
						writer.Append(mantissaSignSymbols->PadPlusPrefix());
						suffix = signSymbols->PadPlusSuffix();
						mantissaSuffix = mantissaSignSymbols->PadPlusSuffix();
						break;
					case B_USE_POSITIVE_SIGN:
						writer.Append(signSymbols->PlusPrefix());
						writer.Append(mantissaSignSymbols->PlusPrefix());
						suffix = signSymbols->PlusSuffix();
						mantissaSuffix = mantissaSignSymbols->PlusSuffix();
						break;
				}
			}
			// round
			int32 exponent = fExponent;
			char digits[kMaxFloatDigitCount];
			int32 integerDigits = max(minIntegerDigits, 1L);
			int32 fractionDigits
				= max(fDigitCount - integerDigits, minFractionDigits);
			fractionDigits = min(fractionDigits, maxFractionDigits);
			int32 digitCount
				= _Round(digits, integerDigits + fractionDigits, exponent);
			fractionDigits = digitCount - integerDigits;
			if (keepTrailingFractionZeros)
				fractionDigits = max(fractionDigits, minFractionDigits);
			fractionDigits = min(fractionDigits, maxFractionDigits);
			// the mantissa
			// integer part
			int32 existingIntegerDigits = min(integerDigits, digitCount);
			for (int i = 0; i < existingIntegerDigits; i++)
				writer.Append(digitSymbols[(int)digits[digitCount - i - 1]]);
			// pad with zeros to get the desired number of integer digits
			if (existingIntegerDigits < integerDigits) {
				writer.Append(digitSymbols,
							  integerDigits - existingIntegerDigits);
			}
			// unless the number is 0, adjust the exponent
			if (!_IsZero(digits, digitCount))
				exponent -= integerDigits - 1;
			// fraction part
			if (fractionDigits > 0 || forceFractionSeparator)
				writer.Append(fractionSeparator);
			int32 existingFractionDigits
				= min(digitCount - integerDigits, fractionDigits);
			for (int i = existingFractionDigits - 1; i >= 0; i--)
				writer.Append(digitSymbols[(int)digits[i]]);
			// pad with zeros to get the desired number of fraction digits
			if (fractionDigits > existingFractionDigits) {
				writer.Append(digitSymbols,
							  fractionDigits - existingFractionDigits);
			}
			writer.Append(mantissaSuffix);
			// the exponent
			writer.Append(exponentSymbol);

			Integer(static_cast<int64>(exponent)).Format(writer, digitSymbols,
				exponentSignSymbols, B_USE_POSITIVE_SIGN, &kNoGroupingInfo, 2);
			// sign suffix
			writer.Append(suffix);
		}

		void FormatFixedPoint(BufferWriter &writer, const Symbol *digitSymbols,
			const Symbol *fractionSeparator,
			const SignSymbols *signSymbols,
			const SignSymbols *mantissaSignSymbols,
			number_format_sign_policy signPolicy,
			const GroupingInfo *groupingInfo,
			int32 minIntegerDigits, int32 minFractionDigits,
			int32 maxFractionDigits, bool forceFractionSeparator,
			bool keepTrailingFractionZeros) const
		{
			const Symbol *suffix = NULL;
			const Symbol *mantissaSuffix = NULL;
			// write sign prefix
			if (fNegative) {
				writer.Append(signSymbols->MinusPrefix());
				writer.Append(mantissaSignSymbols->MinusPrefix());
				suffix = signSymbols->MinusSuffix();
				mantissaSuffix = mantissaSignSymbols->MinusSuffix();
			} else {
				switch (signPolicy) {
					case B_USE_NEGATIVE_SIGN_ONLY:
						writer.Append(signSymbols->NoForcePlusPrefix());
						writer.Append(mantissaSignSymbols->NoForcePlusPrefix());
						suffix = signSymbols->NoForcePlusSuffix();
						mantissaSuffix
							= mantissaSignSymbols->NoForcePlusSuffix();
						break;
					case B_USE_SPACE_FOR_POSITIVE_SIGN:
						writer.Append(signSymbols->PadPlusPrefix());
						writer.Append(mantissaSignSymbols->PadPlusPrefix());
						suffix = signSymbols->PadPlusSuffix();
						mantissaSuffix = mantissaSignSymbols->PadPlusSuffix();
						break;
					case B_USE_POSITIVE_SIGN:
						writer.Append(signSymbols->PlusPrefix());
						writer.Append(mantissaSignSymbols->PlusPrefix());
						suffix = signSymbols->PlusSuffix();
						mantissaSuffix = mantissaSignSymbols->PlusSuffix();
						break;
				}
			}
			// round
			int32 exponent = fExponent;
			char digits[kMaxFloatDigitCount];
			int32 integerDigits = max(minIntegerDigits, exponent + 1L);
			int32 fractionDigits
				= max(fDigitCount - integerDigits, minFractionDigits);
			fractionDigits = min(fractionDigits, maxFractionDigits);
			int32 digitCount
				= _Round(digits, integerDigits + fractionDigits, exponent);
			fractionDigits = digitCount - integerDigits;
			if (keepTrailingFractionZeros)
				fractionDigits = max(fractionDigits, minFractionDigits);
			fractionDigits = min(fractionDigits, maxFractionDigits);
			// integer part
			int32 existingIntegerDigits = min(integerDigits, exponent + 1);
			existingIntegerDigits = max(existingIntegerDigits, 0L);
			if (groupingInfo) {
				// use grouping
				// pad with zeros up to minDigits
				for (int i = integerDigits - 1;
					 i >= existingIntegerDigits;
					 i--) {
					if (i != integerDigits - 1)
						writer.Append(groupingInfo->SeparatorForDigit(i));
					writer.Append(digitSymbols[0]);
				}
				// write digits
				for (int i = existingIntegerDigits - 1; i >= 0; i--) {
					if (i != integerDigits - 1)
						writer.Append(groupingInfo->SeparatorForDigit(i));
					writer.Append(digitSymbols[(int)digits[
						digitCount - existingIntegerDigits + i]]);
				}
			} else {
				// no grouping
				// pad with zeros to get the desired number of integer digits
				if (existingIntegerDigits < integerDigits) {
					writer.Append(digitSymbols[0],
								  integerDigits - existingIntegerDigits);
				}
				// write digits
				for (int i = 0; i < existingIntegerDigits; i++) {
					writer.Append(
						digitSymbols[(int)digits[digitCount - i - 1]]);
				}
			}
			// fraction part
			if (fractionDigits > 0 || forceFractionSeparator)
				writer.Append(fractionSeparator);
			int32 existingFractionDigits
				= min(digitCount - existingIntegerDigits, fractionDigits);
			for (int i = existingFractionDigits - 1; i >= 0; i--)
				writer.Append(digitSymbols[(int)digits[i]]);
			// pad with zeros to get the desired number of fraction digits
			if (fractionDigits > existingFractionDigits) {
				writer.Append(digitSymbols,
							  fractionDigits - existingFractionDigits);
			}
			// sign suffixes
			writer.Append(mantissaSuffix);
			writer.Append(suffix);
		}

	private:
		int32 _Round(char *digits, int32 count, int32 &exponent) const
		{
			if (count > fDigitCount)
				count = fDigitCount;
			int firstNonNull = -1;
// TODO: Non-hardcoded base-independent rounding.
			bool carry = false;
			if (count < fDigitCount)
				carry = (fDigits[fDigitCount - count - 1] >= 5);
			for (int i = 0; i < count; i++) {
				char digit =  fDigits[fDigitCount - count + i];
				if (carry) {
					digit++;
					if (digit == 10)	// == base
						digit = 0;
					else
						carry = false;
				}
				if (firstNonNull < 0 && digit > 0)
					firstNonNull = i;
				if (firstNonNull >= 0)
					digits[i - firstNonNull] = digit;
			}
			if (firstNonNull < 0) {
				if (carry) {
					// 9.999999... -> 10
					digits[0] = 1;
					exponent++;
				} else {
					// 0.0000...1 -> 0
					exponent = 0;
					digits[0] = 0;
				}
				count = 1;
			} else
				count -= firstNonNull;
			return count;
		}

		static bool _IsZero(const char *digits, int32 digitCount)
		{
			return (digitCount == 1 && digits[0] == 0);
		}

	private:
		bool	fNegative;
		int		fClass;
		int32	fExponent;
		char	fDigits[kMaxFloatDigitCount];
		int32	fDigitCount;
};


// constructor
BGenericNumberFormat::BGenericNumberFormat()
	: fIntegerParameters(),
	  fFloatParameters(),
	  fDigitSymbols(NULL),
	  fFractionSeparator(NULL),
	  fGroupingInfo(NULL),
	  fExponentSymbol(NULL),
	  fUpperCaseExponentSymbol(NULL),
	  fNaNSymbol(NULL),
	  fUpperCaseNaNSymbol(NULL),
	  fInfinitySymbol(NULL),
	  fUpperCaseInfinitySymbol(NULL),
	  fNegativeInfinitySymbol(NULL),
	  fUpperCaseNegativeInfinitySymbol(NULL),
	  fSignSymbols(NULL),
	  fMantissaSignSymbols(NULL),
	  fExponentSignSymbols(NULL)
{
}

// destructor
BGenericNumberFormat::~BGenericNumberFormat()
{
	delete fSignSymbols;
	delete fMantissaSignSymbols;
	delete fExponentSignSymbols;
}

// FormatInteger
status_t
BGenericNumberFormat::FormatInteger(
	const BIntegerFormatParameters *parameters, int64 number, BString *buffer,
	format_field_position *positions, int32 positionCount, int32 *fieldCount,
	bool allFieldPositions) const
{
	if (!buffer)
		return B_BAD_VALUE;
	char localBuffer[1024];
	status_t error = FormatInteger(parameters, number, localBuffer,
		sizeof(localBuffer), positions, positionCount, fieldCount,
		allFieldPositions);
	if (error == B_OK) {
		buffer->Append(localBuffer);
		// TODO: Check, if the allocation succeeded.
	}
	return error;
}

// FormatInteger
status_t
BGenericNumberFormat::FormatInteger(
	const BIntegerFormatParameters *parameters, uint64 number, BString *buffer,
	format_field_position *positions, int32 positionCount, int32 *fieldCount,
	bool allFieldPositions) const
{
	if (!buffer)
		return B_BAD_VALUE;
	char localBuffer[1024];
	status_t error = FormatInteger(parameters, number, localBuffer,
		sizeof(localBuffer), positions, positionCount, fieldCount,
		allFieldPositions);
	if (error == B_OK) {
		buffer->Append(localBuffer);
		// TODO: Check, if the allocation succeeded.
	}
	return error;
}

// FormatInteger
status_t
BGenericNumberFormat::FormatInteger(
	const BIntegerFormatParameters *parameters, int64 number, char *buffer,
	size_t bufferSize, format_field_position *positions, int32 positionCount,
	int32 *fieldCount, bool allFieldPositions) const
{
	Integer integer(number);
	return FormatInteger(parameters, integer, buffer, bufferSize, positions,
						 positionCount, fieldCount, allFieldPositions);
}

// FormatInteger
status_t
BGenericNumberFormat::FormatInteger(
	const BIntegerFormatParameters *parameters, uint64 number, char *buffer,
	size_t bufferSize, format_field_position *positions, int32 positionCount,
	int32 *fieldCount, bool allFieldPositions) const
{
	Integer integer(number);
	return FormatInteger(parameters, integer, buffer, bufferSize, positions,
						 positionCount, fieldCount, allFieldPositions);
}

// FormatFloat
status_t
BGenericNumberFormat::FormatFloat(const BFloatFormatParameters *parameters,
	double number, BString *buffer, format_field_position *positions,
	int32 positionCount, int32 *fieldCount, bool allFieldPositions) const
{
	if (!buffer)
		return B_BAD_VALUE;
	// TODO: How to ensure that this is enough?
	char localBuffer[1024];
	status_t error = FormatFloat(parameters, number, localBuffer,
		sizeof(localBuffer), positions, positionCount, fieldCount,
		allFieldPositions);
	if (error == B_OK) {
		buffer->Append(localBuffer);
		// TODO: Check, if the allocation succeeded.
	}
	return error;
}

// FormatFloat
status_t
BGenericNumberFormat::FormatFloat(const BFloatFormatParameters *parameters,
	double number, char *buffer, size_t bufferSize,
	format_field_position *positions, int32 positionCount, int32 *fieldCount,
	bool allFieldPositions) const
{
	// TODO: Check parameters.
	if (!parameters)
		parameters = DefaultFloatFormatParameters();
	if (bufferSize <= parameters->FormatWidth())
		return EOVERFLOW;
	Float floatNumber(number);
	// prepare some parameters
	const GroupingInfo *groupingInfo = NULL;
	if (parameters->UseGrouping())
		groupingInfo = GetGroupingInfo();
	bool upperCase = parameters->UseUpperCase();
	SpecialNumberSymbols specialSymbols;
	GetSpecialNumberSymbols(upperCase, &specialSymbols);
	// compute the length of the formatted string
	BufferWriter writer;
	floatNumber.Format(writer, DigitSymbols(), &specialSymbols,
		FractionSeparator(), ExponentSymbol(), GetSignSymbols(),
		MantissaSignSymbols(), ExponentSignSymbols(),
		parameters->FloatFormatType(), parameters->SignPolicy(), groupingInfo,
		parameters->MinimalIntegerDigits(), parameters->MinimalFractionDigits(),
		parameters->MaximalFractionDigits(),
		parameters->AlwaysUseFractionSeparator(),
		parameters->KeepTrailingFractionZeros());
	int32 stringLength = writer.StringLength();
	int32 charCount = writer.CharCount();
	// consider alignment and check the available space in the buffer
	int32 padding = max(0L, (int32)parameters->FormatWidth() - charCount);
// TODO: Padding with zeros.
	if ((int32)bufferSize <= stringLength + padding)
		return EOVERFLOW;
	// prepare for writing
	writer.SetTo(buffer, bufferSize);
	// write padding for right field alignment
	if (parameters->Alignment() == B_ALIGN_FORMAT_RIGHT && padding > 0)
		writer.Append(' ', padding);
	// write the number
	floatNumber.Format(writer,  DigitSymbols(), &specialSymbols,
		FractionSeparator(), ExponentSymbol(), GetSignSymbols(),
		MantissaSignSymbols(), ExponentSignSymbols(),
		parameters->FloatFormatType(), parameters->SignPolicy(), groupingInfo,
		parameters->MinimalIntegerDigits(), parameters->MinimalFractionDigits(),
		parameters->MaximalFractionDigits(),
		parameters->AlwaysUseFractionSeparator(),
		parameters->KeepTrailingFractionZeros());
	// write padding for left field alignment
	if (parameters->Alignment() == B_ALIGN_FORMAT_LEFT && padding > 0)
		writer.Append(' ', padding);
	return B_OK;
}

// SetDefaultIntegerFormatParameters
status_t
BGenericNumberFormat::SetDefaultIntegerFormatParameters(
	const BIntegerFormatParameters *parameters)
{
	if (!parameters)
		return B_BAD_VALUE;
	fIntegerParameters = *parameters;
	return B_OK;
}

// DefaultIntegerFormatParameters
BIntegerFormatParameters *
BGenericNumberFormat::DefaultIntegerFormatParameters()
{
	return &fIntegerParameters;
}

// DefaultIntegerFormatParameters
const BIntegerFormatParameters *
BGenericNumberFormat::DefaultIntegerFormatParameters() const
{
	return &fIntegerParameters;
}

// SetDefaultFloatFormatParameters
status_t
BGenericNumberFormat::SetDefaultFloatFormatParameters(
	const BFloatFormatParameters *parameters)
{
	if (!parameters)
		return B_BAD_VALUE;
	fFloatParameters = *parameters;
	return B_OK;
}

// DefaultFloatFormatParameters
BFloatFormatParameters *
BGenericNumberFormat::DefaultFloatFormatParameters()
{
	return &fFloatParameters;
}

// DefaultFloatFormatParameters
const BFloatFormatParameters *
BGenericNumberFormat::DefaultFloatFormatParameters() const
{
	return &fFloatParameters;
}

// SetDigitSymbols
status_t
BGenericNumberFormat::SetDigitSymbols(const char **digits)
{
	// check parameters
	if (digits) {
		for (int i = 0; i < 10; i++) {
			if (!digits[i])
				return B_BAD_VALUE;
		}
	}
	// unset old
	if (fDigitSymbols) {
		delete[] fDigitSymbols;
		fDigitSymbols = NULL;
	}
	// set new
	if (digits) {
		fDigitSymbols = new(nothrow) Symbol[10];
		if (!fDigitSymbols)
			return B_NO_MEMORY;
		for (int i = 0; i < 10; i++) {
			status_t error = fDigitSymbols[i].SetTo(digits[i]);
			if (error != B_OK) {
				SetDigitSymbols(NULL);
				return error;
			}
		}
	}
	return B_OK;
}

// SetFractionSeparator
status_t
BGenericNumberFormat::SetFractionSeparator(const char *decimalSeparator)
{
	return _SetSymbol(&fFractionSeparator, decimalSeparator);
}

// SetGroupingInfo
status_t
BGenericNumberFormat::SetGroupingInfo(const char **groupingSeparators,
	size_t separatorCount, size_t *groupingSizes, size_t sizeCount)
{
	// check parameters
	if (groupingSeparators && separatorCount > 0 && groupingSizes
		&& sizeCount) {
		for (int i = 0; i < (int)separatorCount; i++) {
			if (!groupingSeparators[i])
				return B_BAD_VALUE;
		}
	}
	// unset old
	if (fGroupingInfo) {
		delete fGroupingInfo;
		fGroupingInfo = NULL;
	}
	// set new
	if (groupingSeparators && separatorCount > 0 && groupingSizes
		&& sizeCount) {
		fGroupingInfo = new GroupingInfo;
		if (!fGroupingInfo)
			return B_NO_MEMORY;
		status_t error = fGroupingInfo->SetTo(groupingSeparators,
			separatorCount, groupingSizes, sizeCount);
		if (error != B_OK) {
			delete fGroupingInfo;
			fGroupingInfo = NULL;
			return error;
		}
	}
	return B_OK;
}

// SetExponentSymbol
status_t
BGenericNumberFormat::SetExponentSymbol(const char *exponentSymbol,
	const char *upperCaseExponentSymbol)
{
	status_t error = _SetSymbol(&fExponentSymbol, exponentSymbol);
	if (error == B_OK)
		error = _SetSymbol(&fUpperCaseExponentSymbol, upperCaseExponentSymbol);
	if (error != B_OK)
		SetExponentSymbol(NULL, NULL);
	return error;
}

// SetSpecialNumberSymbols
status_t
BGenericNumberFormat::SetSpecialNumberSymbols(const char *nan,
	const char *infinity, const char *negativeInfinity,
	const char *upperCaseNaN, const char *upperCaseInfinity,
	const char *upperCaseNegativeInfinity)
{
	status_t error = _SetSymbol(&fNaNSymbol, nan);
	if (error == B_OK)
		error = _SetSymbol(&fInfinitySymbol, infinity);
	if (error == B_OK)
		error = _SetSymbol(&fNegativeInfinitySymbol, negativeInfinity);
	if (error == B_OK)
		error = _SetSymbol(&fUpperCaseNaNSymbol, upperCaseNaN);
	if (error == B_OK)
		error = _SetSymbol(&fUpperCaseInfinitySymbol, upperCaseInfinity);
	if (error == B_OK) {
		error = _SetSymbol(&fUpperCaseNegativeInfinitySymbol,
			upperCaseNegativeInfinity);
	}
	if (error != B_OK)
		SetSpecialNumberSymbols(NULL, NULL, NULL, NULL, NULL, NULL);
	return error;
}

// SetSignSymbols
status_t
BGenericNumberFormat::SetSignSymbols(const char *plusPrefix,
	const char *minusPrefix, const char *padPlusPrefix,
	const char *noForcePlusPrefix, const char *plusSuffix,
	const char *minusSuffix, const char *padPlusSuffix,
	const char *noForcePlusSuffix)
{
	if (!fSignSymbols) {
		fSignSymbols = new(nothrow) SignSymbols;
		if (!fSignSymbols)
			return B_NO_MEMORY;
	}
	return fSignSymbols->SetTo(plusPrefix, minusPrefix, padPlusPrefix,
		noForcePlusPrefix, plusSuffix, minusSuffix, padPlusSuffix,
		noForcePlusSuffix);
}

// SetMantissaSignSymbols
status_t
BGenericNumberFormat::SetMantissaSignSymbols(const char *plusPrefix,
	const char *minusPrefix, const char *padPlusPrefix,
	const char *noForcePlusPrefix, const char *plusSuffix,
	const char *minusSuffix, const char *padPlusSuffix,
	const char *noForcePlusSuffix)
{
	if (!fMantissaSignSymbols) {
		fMantissaSignSymbols = new(nothrow) SignSymbols;
		if (!fMantissaSignSymbols)
			return B_NO_MEMORY;
	}
	return fMantissaSignSymbols->SetTo(plusPrefix, minusPrefix, padPlusPrefix,
		noForcePlusPrefix, plusSuffix, minusSuffix, padPlusSuffix,
		noForcePlusSuffix);
}

// SetExponentSignSymbols
status_t
BGenericNumberFormat::SetExponentSignSymbols(const char *plusPrefix,
	const char *minusPrefix, const char *plusSuffix, const char *minusSuffix)
{
	if (!fExponentSignSymbols) {
		fExponentSignSymbols = new(nothrow) SignSymbols;
		if (!fExponentSignSymbols)
			return B_NO_MEMORY;
	}
	return fExponentSignSymbols->SetTo(plusPrefix, minusPrefix, plusPrefix,
		plusPrefix, plusSuffix, minusSuffix, plusSuffix, plusSuffix);
}

// FormatInteger
status_t
BGenericNumberFormat::FormatInteger(
	const BIntegerFormatParameters *parameters, const Integer &integer,
	char *buffer, size_t bufferSize, format_field_position *positions,
	int32 positionCount, int32 *fieldCount, bool allFieldPositions) const
{
	// TODO: Check parameters.
	if (!parameters)
		parameters = DefaultIntegerFormatParameters();
	if (bufferSize <= parameters->FormatWidth())
		return EOVERFLOW;
	// prepare some parameters
	const GroupingInfo *groupingInfo = NULL;
	if (parameters->UseGrouping())
		groupingInfo = GetGroupingInfo();
	// compute the length of the formatted string
	BufferWriter writer;
	integer.Format(writer, DigitSymbols(),
		GetSignSymbols(), parameters->SignPolicy(), groupingInfo,
		parameters->MinimalIntegerDigits());
	int32 stringLength = writer.StringLength();
	int32 charCount = writer.CharCount();
	// consider alignment and check the available space in the buffer
	int32 padding = max(0L, (int32)parameters->FormatWidth() - charCount);
// TODO: Padding with zeros.
	if ((int32)bufferSize <= stringLength + padding)
		return EOVERFLOW;
	// prepare for writing
	writer.SetTo(buffer, bufferSize);
	// write padding for right field alignment
	if (parameters->Alignment() == B_ALIGN_FORMAT_RIGHT && padding > 0)
		writer.Append(' ', padding);
	// write the number
	integer.Format(writer, DigitSymbols(),
		GetSignSymbols(), parameters->SignPolicy(), groupingInfo,
		parameters->MinimalIntegerDigits());
	// write padding for left field alignment
	if (parameters->Alignment() == B_ALIGN_FORMAT_LEFT && padding > 0)
		writer.Append(' ', padding);
	return B_OK;
}

// DigitSymbols
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::DigitSymbols() const
{
	return (fDigitSymbols ? fDigitSymbols : kDefaultDigitSymbols);
}

// FractionSeparator
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::FractionSeparator() const
{
	return (fFractionSeparator ? fFractionSeparator
							   : &kDefaultFractionSeparator);
}

// GetGroupingInfo
const BGenericNumberFormat::GroupingInfo *
BGenericNumberFormat::GetGroupingInfo() const
{
	return (fGroupingInfo ? fGroupingInfo : &kDefaultGroupingInfo);
}

// ExponentSymbol
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::ExponentSymbol(bool upperCase) const
{
	if (fExponentSymbol) {
		return (upperCase && fUpperCaseExponentSymbol ? fUpperCaseExponentSymbol
													  : fExponentSymbol);
	}
	return (upperCase ? &kDefaultUpperCaseExponentSymbol
					  : &kDefaultExponentSymbol);
}

// NaNSymbol
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::NaNSymbol(bool upperCase) const
{
	if (fNaNSymbol) {
		return (upperCase && fUpperCaseNaNSymbol ? fUpperCaseNaNSymbol
												 : fNaNSymbol);
	}
	return (upperCase ? &kDefaultUpperCaseNaNSymbol
					  : &kDefaultNaNSymbol);
}

// InfinitySymbol
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::InfinitySymbol(bool upperCase) const
{
	if (fInfinitySymbol) {
		return (upperCase && fUpperCaseInfinitySymbol ? fUpperCaseInfinitySymbol
													  : fInfinitySymbol);
	}
	return (upperCase ? &kDefaultUpperCaseInfinitySymbol
					  : &kDefaultInfinitySymbol);
}

// NegativeInfinitySymbol
const BGenericNumberFormat::Symbol *
BGenericNumberFormat::NegativeInfinitySymbol(bool upperCase) const
{
	if (fNegativeInfinitySymbol) {
		return (upperCase && fUpperCaseNegativeInfinitySymbol
			? fUpperCaseNegativeInfinitySymbol : fNegativeInfinitySymbol);
	}
	return (upperCase ? &kDefaultUpperCaseNegativeInfinitySymbol
					  : &kDefaultNegativeInfinitySymbol);
}

// GetSpecialNumberSymbols
void
BGenericNumberFormat::GetSpecialNumberSymbols(bool upperCase,
	SpecialNumberSymbols *symbols) const
{
	symbols->nan = NaNSymbol(upperCase);
	symbols->infinity = InfinitySymbol(upperCase);
	symbols->negative_infinity = NegativeInfinitySymbol(upperCase);
}

// GetSignSymbols
const BGenericNumberFormat::SignSymbols *
BGenericNumberFormat::GetSignSymbols() const
{
	return (fSignSymbols ? fSignSymbols : &kDefaultSignSymbols);
}

// MantissaSignSymbols
const BGenericNumberFormat::SignSymbols *
BGenericNumberFormat::MantissaSignSymbols() const
{
	return (fMantissaSignSymbols ? fMantissaSignSymbols
								 : &kDefaultMantissaSignSymbols);
}

// ExponentSignSymbols
const BGenericNumberFormat::SignSymbols *
BGenericNumberFormat::ExponentSignSymbols() const
{
	return (fExponentSignSymbols ? fExponentSignSymbols
								 : &kDefaultExponentSignSymbols);
}

// _SetSymbol
status_t
BGenericNumberFormat::_SetSymbol(Symbol **symbol, const char *str)
{
	if (!str) {
		// no symbol -- unset old
		if (*symbol) {
			delete *symbol;
			symbol = NULL;
		}
	} else {
		// allocate if not existing
		if (!*symbol) {
			*symbol = new(nothrow) Symbol;
			if (!*symbol)
				return B_NO_MEMORY;
		}
		// set symbol
		status_t error = (*symbol)->SetTo(str);
		if (error != B_OK) {
			delete *symbol;
			*symbol = NULL;
			return B_NO_MEMORY;
		}
	}
	return B_OK;
}

