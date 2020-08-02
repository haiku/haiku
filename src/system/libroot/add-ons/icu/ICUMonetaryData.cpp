/*
 * Copyright 2010-2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUMonetaryData.h"

#include <langinfo.h>
#include <limits.h>
#include <string.h>
#include <strings.h>


U_NAMESPACE_USE


namespace BPrivate {
namespace Libroot {


ICUMonetaryData::ICUMonetaryData(pthread_key_t tlsKey, struct lconv& localeConv)
	:
	inherited(tlsKey),
	fLocaleConv(localeConv),
	fPosixLocaleConv(NULL)
{
	fLocaleConv.int_curr_symbol = fIntCurrSymbol;
	fLocaleConv.currency_symbol = fCurrencySymbol;
	fLocaleConv.mon_decimal_point = fDecimalPoint;
	fLocaleConv.mon_thousands_sep = fThousandsSep;
	fLocaleConv.mon_grouping = fGrouping;
	fLocaleConv.positive_sign = fPositiveSign;
	fLocaleConv.negative_sign = fNegativeSign;
}


void
ICUMonetaryData::Initialize(LocaleMonetaryDataBridge* dataBridge)
{
	fPosixLocaleConv = dataBridge->posixLocaleConv;
}


status_t
ICUMonetaryData::SetTo(const Locale& locale, const char* posixLocaleName)
{
	status_t result = inherited::SetTo(locale, posixLocaleName);

	if (result == B_OK) {
		UErrorCode icuStatus = U_ZERO_ERROR;
		UChar intlCurrencySeparatorChar = CHAR_MAX;
		DecimalFormat* currencyFormat = dynamic_cast<DecimalFormat*>(
			NumberFormat::createCurrencyInstance(locale, icuStatus));
		if (!U_SUCCESS(icuStatus))
			return B_UNSUPPORTED;
		if (!currencyFormat)
			return B_BAD_TYPE;

		const DecimalFormatSymbols* formatSymbols
			= currencyFormat->getDecimalFormatSymbols();
		if (!formatSymbols)
			result = B_BAD_DATA;

		if (result == B_OK) {
			result = _SetLocaleconvEntry(formatSymbols, fDecimalPoint,
				DecimalFormatSymbols::kMonetarySeparatorSymbol);
		}
		if (result == B_OK) {
			result = _SetLocaleconvEntry(formatSymbols, fThousandsSep,
				DecimalFormatSymbols::kMonetaryGroupingSeparatorSymbol);
		}
		if (result == B_OK) {
			int32 groupingSize = currencyFormat->getGroupingSize();
			if (groupingSize < 1)
				fGrouping[0] = '\0';
			else {
				fGrouping[0] = groupingSize;
				int32 secondaryGroupingSize
					= currencyFormat->getSecondaryGroupingSize();
				if (secondaryGroupingSize < 1)
					fGrouping[1] = '\0';
				else {
					fGrouping[1] = secondaryGroupingSize;
					fGrouping[2] = '\0';
				}
			}
		}
		if (result == B_OK) {
			fLocaleConv.int_frac_digits
				= currencyFormat->getMinimumFractionDigits();
			fLocaleConv.frac_digits
				= currencyFormat->getMinimumFractionDigits();
		}
		if (result == B_OK) {
			UnicodeString positivePrefix, positiveSuffix, negativePrefix,
				negativeSuffix;
			currencyFormat->getPositivePrefix(positivePrefix);
			currencyFormat->getPositiveSuffix(positiveSuffix);
			currencyFormat->getNegativePrefix(negativePrefix);
			currencyFormat->getNegativeSuffix(negativeSuffix);
			UnicodeString currencySymbol = formatSymbols->getSymbol(
				DecimalFormatSymbols::kCurrencySymbol);
			UnicodeString plusSymbol = formatSymbols->getSymbol(
				DecimalFormatSymbols::kPlusSignSymbol);
			UnicodeString minusSymbol = formatSymbols->getSymbol(
				DecimalFormatSymbols::kMinusSignSymbol);

			// fill national values
			int32 positiveCurrencyFlags = _DetermineCurrencyPosAndSeparator(
				positivePrefix, positiveSuffix, plusSymbol, currencySymbol,
				intlCurrencySeparatorChar);
			fLocaleConv.p_cs_precedes
				= (positiveCurrencyFlags & kCsPrecedesFlag) ? 1 : 0;
			fLocaleConv.p_sep_by_space
				= (positiveCurrencyFlags & kSepBySpaceFlag) ? 1 : 0;

			int32 negativeCurrencyFlags = _DetermineCurrencyPosAndSeparator(
				negativePrefix, negativeSuffix, minusSymbol, currencySymbol,
				intlCurrencySeparatorChar);
			fLocaleConv.n_cs_precedes
				= (negativeCurrencyFlags & kCsPrecedesFlag) ? 1 : 0;
			fLocaleConv.n_sep_by_space
				= (negativeCurrencyFlags & kSepBySpaceFlag) ? 1 : 0;

			fLocaleConv.p_sign_posn = _DetermineSignPos(positivePrefix,
				positiveSuffix, plusSymbol, currencySymbol);
			fLocaleConv.n_sign_posn = _DetermineSignPos(negativePrefix,
				negativeSuffix, minusSymbol, currencySymbol);
			if (fLocaleConv.p_sign_posn == CHAR_MAX) {
				// usually there is no positive sign indicator, so we
				// adopt the sign pos of the negative sign symbol
				fLocaleConv.p_sign_posn = fLocaleConv.n_sign_posn;
			}

			// copy national to international values, as ICU does not seem
			// to have separate info for those
			fLocaleConv.int_p_cs_precedes = fLocaleConv.p_cs_precedes;
			fLocaleConv.int_p_sep_by_space = fLocaleConv.p_sep_by_space;
			fLocaleConv.int_n_cs_precedes = fLocaleConv.n_cs_precedes;
			fLocaleConv.int_n_sep_by_space = fLocaleConv.n_sep_by_space;
			fLocaleConv.int_p_sign_posn = fLocaleConv.p_sign_posn;
			fLocaleConv.int_n_sign_posn = fLocaleConv.n_sign_posn;

			// only set sign symbols if they are actually used in any pattern
			if (positivePrefix.indexOf(plusSymbol) > -1
				|| positiveSuffix.indexOf(plusSymbol) > -1) {
				result = _SetLocaleconvEntry(formatSymbols, fPositiveSign,
					DecimalFormatSymbols::kPlusSignSymbol);
			} else
				fPositiveSign[0] = '\0';
			if (negativePrefix.indexOf(minusSymbol) > -1
				|| negativeSuffix.indexOf(minusSymbol) > -1) {
				result = _SetLocaleconvEntry(formatSymbols, fNegativeSign,
					DecimalFormatSymbols::kMinusSignSymbol);
			} else
				fNegativeSign[0] = '\0';
		}
		if (result == B_OK) {
			UnicodeString intlCurrencySymbol = formatSymbols->getSymbol(
				DecimalFormatSymbols::kIntlCurrencySymbol);
			if (intlCurrencySeparatorChar != CHAR_MAX)
				intlCurrencySymbol += intlCurrencySeparatorChar;
			else
				intlCurrencySymbol += ' ';
			result = _ConvertUnicodeStringToLocaleconvEntry(intlCurrencySymbol,
				fIntCurrSymbol, skLCBufSize);
		}
		if (result == B_OK) {
			result = _SetLocaleconvEntry(formatSymbols, fCurrencySymbol,
				DecimalFormatSymbols::kCurrencySymbol);
			if (fCurrencySymbol[0] == '\0') {
				// fall back to the international currency symbol
				result = _SetLocaleconvEntry(formatSymbols, fCurrencySymbol,
					DecimalFormatSymbols::kIntlCurrencySymbol);
				fCurrencySymbol[3] = '\0';
					// drop separator char that's contained in int-curr-symbol
			}
		}

		delete currencyFormat;
	}

	return result;
}


status_t
ICUMonetaryData::SetToPosix()
{
	status_t result = inherited::SetToPosix();

	if (result == B_OK) {
		strcpy(fDecimalPoint, fPosixLocaleConv->mon_decimal_point);
		strcpy(fThousandsSep, fPosixLocaleConv->mon_thousands_sep);
		strcpy(fGrouping, fPosixLocaleConv->mon_grouping);
		strcpy(fCurrencySymbol, fPosixLocaleConv->currency_symbol);
		strcpy(fIntCurrSymbol, fPosixLocaleConv->int_curr_symbol);
		strcpy(fPositiveSign, fPosixLocaleConv->positive_sign);
		strcpy(fNegativeSign, fPosixLocaleConv->negative_sign);
		fLocaleConv.int_frac_digits = fPosixLocaleConv->int_frac_digits;
		fLocaleConv.frac_digits = fPosixLocaleConv->frac_digits;
		fLocaleConv.p_cs_precedes = fPosixLocaleConv->p_cs_precedes;
		fLocaleConv.p_sep_by_space = fPosixLocaleConv->p_sep_by_space;
		fLocaleConv.n_cs_precedes = fPosixLocaleConv->n_cs_precedes;
		fLocaleConv.n_sep_by_space = fPosixLocaleConv->n_sep_by_space;
		fLocaleConv.p_sign_posn = fPosixLocaleConv->p_sign_posn;
		fLocaleConv.n_sign_posn = fPosixLocaleConv->n_sign_posn;
		fLocaleConv.int_p_cs_precedes = fPosixLocaleConv->int_p_cs_precedes;
		fLocaleConv.int_p_sep_by_space = fPosixLocaleConv->int_p_sep_by_space;
		fLocaleConv.int_n_cs_precedes = fPosixLocaleConv->int_n_cs_precedes;
		fLocaleConv.int_n_sep_by_space = fPosixLocaleConv->int_n_sep_by_space;
		fLocaleConv.int_p_sign_posn = fPosixLocaleConv->int_p_sign_posn;
		fLocaleConv.int_n_sign_posn = fPosixLocaleConv->int_n_sign_posn;
	}

	return result;
}


const char*
ICUMonetaryData::GetLanginfo(int index)
{
	switch(index) {
		case CRNCYSTR:
			return fCurrencySymbol;
		default:
			return "";
	}
}


int32
ICUMonetaryData::_DetermineCurrencyPosAndSeparator(const UnicodeString& prefix,
	const UnicodeString& suffix, const UnicodeString& signSymbol,
	const UnicodeString& currencySymbol, UChar& currencySeparatorChar)
{
	int32 result = 0;

	int32 currencySymbolPos = prefix.indexOf(currencySymbol);
	if (currencySymbolPos > -1) {
		result |= kCsPrecedesFlag;
		int32 signSymbolPos = prefix.indexOf(signSymbol);
		// if a char is following the currency symbol, we assume it's
		// the separator (usually space), but we need to take care to
		// skip over the sign symbol, if found
		int32 potentialSeparatorPos
			= currencySymbolPos + currencySymbol.length();
		if (potentialSeparatorPos == signSymbolPos)
			potentialSeparatorPos++;
		if (prefix.charAt(potentialSeparatorPos) != 0xFFFF) {
			// We can't use the actual separator char since this is usually
			// 'c2a0' (non-breakable space), which is not available in the
			// ASCII charset used/assumed by POSIX lconv. So we use space
			// instead.
			currencySeparatorChar = ' ';
			result |= kSepBySpaceFlag;
		}
	} else {
		currencySymbolPos = suffix.indexOf(currencySymbol);
		if (currencySymbolPos > -1) {
			int32 signSymbolPos = suffix.indexOf(signSymbol);
			// if a char is preceding the currency symbol, we assume
			// it's the separator (usually space), but we need to take
			// care to skip the sign symbol, if found
			int32 potentialSeparatorPos = currencySymbolPos - 1;
			if (potentialSeparatorPos == signSymbolPos)
				potentialSeparatorPos--;
			if (suffix.charAt(potentialSeparatorPos) != 0xFFFF) {
				// We can't use the actual separator char since this is usually
				// 'c2a0' (non-breakable space), which is not available in the
				// ASCII charset used/assumed by POSIX lconv. So we use space
				// instead.
				currencySeparatorChar = ' ';
				result |= kSepBySpaceFlag;
			}
		}
	}

	return result;
}


/*
 * This method determines the positive/negative sign position value according to
 * the following map (where '$' indicated the currency symbol, '#' the number
 * value, and '-' or parantheses the sign symbol):
 *		($#)	-> 	0
 *		(#$) 	->	0
 *		-$# 	->	1
 *		-#$		->	1
 *		$-#		->	4
 *		$#-		->	2
 *		#$-		->	2
 *		#-$		->	3
 */
int32
ICUMonetaryData::_DetermineSignPos(const UnicodeString& prefix,
	const UnicodeString& suffix, const UnicodeString& signSymbol,
	const UnicodeString& currencySymbol)
{
	if (prefix.indexOf(UnicodeString("(", "")) >= 0
		&& suffix.indexOf(UnicodeString(")", "")) >= 0)
		return kParenthesesAroundCurrencyAndValue;

	UnicodeString value("#", "");
	UnicodeString prefixNumberSuffixString = prefix + value + suffix;
	int32 signSymbolPos = prefixNumberSuffixString.indexOf(signSymbol);
	if (signSymbolPos >= 0) {
		int32 valuePos = prefixNumberSuffixString.indexOf(value);
		int32 currencySymbolPos
			= prefixNumberSuffixString.indexOf(currencySymbol);

		if (signSymbolPos < valuePos && signSymbolPos < currencySymbolPos)
			return kSignPrecedesCurrencyAndValue;
		if (signSymbolPos > valuePos && signSymbolPos > currencySymbolPos)
			return kSignSucceedsCurrencyAndValue;
		if (signSymbolPos == currencySymbolPos - 1)
			return kSignImmediatelyPrecedesCurrency;
		if (signSymbolPos == currencySymbolPos + currencySymbol.length())
			return kSignImmediatelySucceedsCurrency;
	}

	return CHAR_MAX;
}


}	// namespace Libroot
}	// namespace BPrivate
