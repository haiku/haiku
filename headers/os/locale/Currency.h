#ifndef _B_CURRENCY_H_
#define _B_CURRENCY_H_

#include <Archivable.h>
#include <Message.h>
#include <String.h>

class BLocale;

class BCurrency : public BArchivable {
	public:
		BCurrency(const BCurrency &other);
		BCurrency(BMessage *archive);
		BCurrency(const char *currencyCode);
		~BCurrency();

		status_t InitCheck() const;

		virtual status_t Archive(BMessage *archive, bool deep = true) const;
		static BArchivable *Instantiate(BMessage *archive);

		const char *CurrencyCode() const;
		const char *DefaultSymbol() const;
		int32 DefaultFractionDigits() const;

		status_t GetSymbol(char *symbol, size_t maxSize,
						   BLocale *locale = NULL);
		status_t GetSymbol(BString *symbol, BLocale *locale = NULL);

		BCurrency &operator=(const BCurrency &other);
		bool operator==(const BCurrency &other) const;
		bool operator!=(const BCurrency &other) const;

	private:
		BCurrency();

		bool _CheckData() const;
		void _Unset(status_t error);

		BString	fCurrencyCode;
		BString	fDefaultSymbol;
		int32	fDefaultFractionDigits;
};

#endif	// _B_CURRENCY_H_
