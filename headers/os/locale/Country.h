#ifndef _COUNTRY_H_
#define _COUNTRY_H_


#include <SupportDefs.h>
#include <LocaleBuild.h>
#include <LocaleStrings.h>
#include <String.h>

enum {
	B_METRIC = 0,
	B_US
};


class _IMPEXP_LOCALE BCountry {
	public:
		BCountry();
		virtual ~BCountry();
		
		virtual const char *Name() const;

		// see definitions below
		const char *GetString(uint32 id) const;

		// date & time

		virtual void	FormatDate(char *string,size_t maxSize,time_t time,bool longFormat);
		virtual void	FormatDate(BString *string,time_t time,bool longFormat);
		virtual void	FormatTime(char *string,size_t maxSize,time_t time,bool longFormat);
		virtual void	FormatTime(BString *string,time_t time,bool longFormat);

		const char		*DateFormat(bool longFormat) const;
		const char		*TimeFormat(bool longFormat) const;
		const char		*DateSeparator() const;
		const char		*TimeSeparator() const;

		// numbers

		virtual void	FormatNumber(char *string,size_t maxSize,double value);
		virtual void	FormatNumber(BString *string,double value);
		virtual void	FormatNumber(char *string,size_t maxSize,int32 value);
		virtual void	FormatNumber(BString *string,int32 value);

		const char		*DecimalPoint() const;
		const char		*ThousandsSeparator() const;
		const char		*Grouping() const;

		const char		*PositiveSign() const;
		const char		*NegativeSign() const;

		// measurements

		virtual int8	Measurement() const;

		// monetary

		virtual ssize_t	FormatMonetary(char *string,size_t maxSize,char *format, ...);
		virtual ssize_t	FormatMonetary(BString *string,char *format, ...);

		const char		*CurrencySymbol() const;
		const char		*InternationalCurrencySymbol() const;
		const char		*MonDecimalPoint() const;
		const char		*MonThousandsSeparator() const;
		const char		*MonGrouping() const;
		virtual int32	MonFracDigits() const;

	protected:
		BCountry(const char **strings);

	private:
		const char	**fStrings;
};

#endif	/* _COUNTRY_H_ */
