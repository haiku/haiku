// Warnings.h

#include "Warnings.h"

#include <stdio.h>

// constructor
Warnings::Warnings()
		: fWarnings()
{
}

// destructor
Warnings::~Warnings()
{
	for (int32 i = 0; BString* warning = (BString*)fWarnings.ItemAt(i); i++)
		delete warning;
}

// GetCurrentWarnings
Warnings*
Warnings::GetCurrentWarnings()
{
	return fCurrentWarnings;
}

// SetCurrentWarnings
void
Warnings::SetCurrentWarnings(Warnings* warnings)
{
	fCurrentWarnings = warnings;
}

// AddWarning
void
Warnings::AddWarning(BString warning)
{
	fWarnings.AddItem(new BString(warning));
}

// AddWarning
void
Warnings::AddWarning(const char* format,...)
{
	va_list args;
	va_start(args, format);
	AddWarningV(format, args);
	va_end(args);
}

// AddWarningV
void
Warnings::AddWarningV(const char* format, va_list arg)
{
	char buffer[2048];
	vsprintf(buffer, format, arg);
	AddWarning(BString(buffer));
}

// AddCurrentWarning
void
Warnings::AddCurrentWarning(BString warning)
{
	if (Warnings* currentWarnings = GetCurrentWarnings())
		currentWarnings->AddWarning(warning);
}

// AddCurrentWarning
void
Warnings::AddCurrentWarning(const char* format,...)
{
	va_list args;
	va_start(args, format);
	AddCurrentWarningV(format, args);
	va_end(args);
}

// AddCurrentWarningV
void
Warnings::AddCurrentWarningV(const char* format, va_list arg)
{
	char buffer[2048];
	vsprintf(buffer, format, arg);
	AddCurrentWarning(BString(buffer));
}

// CountWarnings
int32
Warnings::CountWarnings() const
{
	return fWarnings.CountItems();
}

// WarningAt
const char*
Warnings::WarningAt(int32 index) const
{
	const char* result = NULL;
	if (BString* warning = (BString*)fWarnings.ItemAt(index))
		result = warning->String();
	return result;
}

// fCurrentWarnings
Warnings* Warnings::fCurrentWarnings = NULL;

