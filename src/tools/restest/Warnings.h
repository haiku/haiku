// Warnings.h

#ifndef WARNINGS_H
#define WARNINGS_H

#include <stdarg.h>

#include <List.h>
#include <String.h>

class Warnings {
public:
								Warnings();
	virtual						~Warnings();

	static	Warnings*			GetCurrentWarnings();
	static	void				SetCurrentWarnings(Warnings* warnings);

			void				AddWarning(BString warning);
			void				AddWarning(const char* format,...);
			void				AddWarningV(const char* format, va_list arg);
	static	void				AddCurrentWarning(BString warning);
	static	void				AddCurrentWarning(const char* format,...);
	static	void				AddCurrentWarningV(const char* format,
												   va_list arg);

			int32				CountWarnings() const;
			const char*			WarningAt(int32 index) const;

private:
	static	Warnings*			fCurrentWarnings;
			BList				fWarnings;
};

#endif	// WARNINGS_H
