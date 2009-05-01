#include <Currency.h>

// message archive field names
static const char *kArchivedCurrencyCodeName	= "be:currency code";
static const char *kArchivedDefaultSymbol		= "be:default symbol";
static const char *kArchivedDefaultFractionDigits
	= "be:default fraction digits";

// constructor
BCurrency::BCurrency(const BCurrency &other)
	: fCurrencyCode(),
	  fDefaultSymbol(),
	  fDefaultFractionDigits(B_NO_INIT)
{
	*this = other;
}

// constructor
BCurrency::BCurrency(BMessage *archive)
	: fCurrencyCode(),
	  fDefaultSymbol(),
	  fDefaultFractionDigits(B_NO_INIT)
{
	if (archive->FindString(kArchivedCurrencyCodeName, &fCurrencyCode)
			== B_OK
		&& archive->FindString(kArchivedDefaultSymbol, &fDefaultSymbol)
			== B_OK
		&& archive->FindInt32(kArchivedDefaultFractionDigits,
							  &fDefaultFractionDigits) == B_OK
		&& _CheckData()) {
		// everything went fine
	} else
		_Unset(B_NO_INIT);
}

// constructor
BCurrency::BCurrency(const char *currencyCode)
{
	// TODO: load currency from disk
}

// destructor
BCurrency::~BCurrency()
{
}

// InitCheck
status_t
BCurrency::InitCheck() const
{
	return (fDefaultFractionDigits < 0 ? fDefaultFractionDigits : B_OK);
}

// Archive
status_t
BCurrency::Archive(BMessage *archive, bool deep) const
{
	status_t error = BArchivable::Archive(archive, deep);
	if (error == B_OK) {
		if (archive->AddString(kArchivedCurrencyCodeName, fCurrencyCode)
				== B_OK
			&& archive->AddString(kArchivedDefaultSymbol, fDefaultSymbol)
				== B_OK
			&& archive->AddInt32(kArchivedDefaultFractionDigits,
								 fDefaultFractionDigits) == B_OK) {
			// everything went fine
		} else
			error = B_ERROR;
	}
	return error;
}

// Instantiate
_EXPORT
BArchivable *
BCurrency::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "BCurrency"))
		return NULL;
	return new BCurrency(archive);
}

// CurrencyCode
const char *
BCurrency::CurrencyCode() const
{
	return (InitCheck() == B_OK ? fCurrencyCode.String() : NULL);
}

// DefaultSymbol
const char *
BCurrency::DefaultSymbol() const
{
	return (InitCheck() == B_OK ? fDefaultSymbol.String() : NULL);
}

// DefaultFractionDigits
int32
BCurrency::DefaultFractionDigits() const
{
	return (InitCheck() == B_OK ? fDefaultFractionDigits : 0);
}

// GetSymbol
status_t
BCurrency::GetSymbol(char *symbol, size_t maxSize, BLocale *locale)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		return B_NO_INIT;
	if (symbol == NULL)
		return B_BAD_VALUE;
	// TODO: get symbol from locale
	// fall back to the default symbol
	if ((int32)maxSize <= fDefaultSymbol.Length())
		return EOVERFLOW;	// OpenBeOS: B_BUFFER_OVERFLOW
	strcpy(symbol, fDefaultSymbol.String());
	return B_OK;
}

// GetSymbol
status_t
BCurrency::GetSymbol(BString *symbol, BLocale *locale)
{
	// check initialization and parameters
	if (InitCheck() != B_OK)
		return B_NO_INIT;
	if (symbol == NULL)
		return B_BAD_VALUE;
	// TODO: get symbol from locale
	// fall back to the default symbol
	*symbol = fDefaultSymbol;
	if (symbol->Length() != fDefaultSymbol.Length())
		return B_NO_MEMORY;
	return B_OK;
}

// =
BCurrency &
BCurrency::operator=(const BCurrency &other)
{
	if (other.InitCheck() == B_OK) {
		fCurrencyCode = other.fCurrencyCode;
		fDefaultSymbol = other.fDefaultSymbol;
		fDefaultFractionDigits = other.fDefaultFractionDigits;
		if (!_CheckData())
			_Unset(B_NO_MEMORY);
	} else
		_Unset(B_NO_MEMORY);
	return *this;
}

// ==
bool
BCurrency::operator==(const BCurrency &other) const
{
	if (InitCheck() != B_OK || other.InitCheck() != B_OK)
		return false;
	return (fCurrencyCode == other.fCurrencyCode);
}

// !=
bool
BCurrency::operator!=(const BCurrency &other) const
{
	return !(*this == other);
}

// constructor
BCurrency::BCurrency()
{
	// privatized to make it unaccessible
}

// _CheckData
bool
BCurrency::_CheckData() const
{
	return (fDefaultFractionDigits >= 0
		&& fCurrencyCode.Length() > 0
		&& fDefaultSymbol.Length() == 0);
}

// _Unset
void
BCurrency::_Unset(status_t error)
{
	fCurrencyCode.Truncate(0);
	fDefaultSymbol.Truncate(0);
	fDefaultFractionDigits = error;
}

