/* 
** Copyright 2004, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

#include <stdio.h>
#include <string.h>

#include <GenericNumberFormat.h>
#include <IntegerFormatParameters.h>

void
format_integer(const char *test, const BGenericNumberFormat &format,
			  const BIntegerFormatParameters *parameters, int64 number)
{
	const int bufferSize = 1024;
	char buffer[bufferSize];
	status_t error = format.FormatInteger(parameters, number, buffer,
		bufferSize);
	if (error == B_OK)
		printf("%-20s `%s'\n", test, buffer);
	else
		printf("%-20s Failed to format number: %s\n", test, strerror(error));
}

void
format_integer(BGenericNumberFormat &format, int64 number)
{
	printf("number: %lld\n", number);
	BIntegerFormatParameters parameters(
		format.DefaultIntegerFormatParameters());
	format_integer("  default:", format, &parameters, number);
	parameters.SetFormatWidth(25);
	format_integer("  right aligned:", format, &parameters, number);
	parameters.SetAlignment(B_ALIGN_FORMAT_LEFT);
	format_integer("  left aligned:", format, &parameters, number);
	parameters.SetFormatWidth(1);
	parameters.SetAlignment(B_ALIGN_FORMAT_RIGHT);
	parameters.SetSignPolicy(B_USE_POSITIVE_SIGN);
	format_integer("  use plus:", format, &parameters, number);
	parameters.SetSignPolicy(B_USE_SPACE_FOR_POSITIVE_SIGN);
	format_integer("  space for plus:", format, &parameters, number);
	parameters.SetSignPolicy(B_USE_NEGATIVE_SIGN_ONLY);
	parameters.SetMinimalIntegerDigits(0);
	format_integer("  min digits 0:", format, &parameters, number);
	parameters.SetMinimalIntegerDigits(5);
	format_integer("  min digits 5:", format, &parameters, number);
	parameters.SetMinimalIntegerDigits(20);
	format_integer("  min digits 20:", format, &parameters, number);
	parameters.SetMinimalIntegerDigits(30);
	format_integer("  min digits 30:", format, &parameters, number);
}

void
format_float(const char *test, const BGenericNumberFormat &format,
			 const BFloatFormatParameters *parameters, double number)
{
	const int bufferSize = 1024;
	char buffer[bufferSize];
	status_t error = format.FormatFloat(parameters, number, buffer, bufferSize);
	if (error == B_OK)
		printf("%-20s `%s'\n", test, buffer);
	else
		printf("%-20s Failed to format number: %s\n", test, strerror(error));
}

void
format_float(BGenericNumberFormat &format, double number)
{
	printf("number: %g\n", number);
	BFloatFormatParameters parameters( format.DefaultFloatFormatParameters());
	format_float("  default:", format, &parameters, number);
}

int
main()
{
	BGenericNumberFormat format;
	printf("Integer\n");
	printf("=======\n\n");
	format_integer(format, 0);
	format_integer(format, 1234567890LL);
	format_integer(format, -1234567890LL);
	format.DefaultIntegerFormatParameters()->SetUseGrouping(true);
	format_integer(format, 0);
	format_integer(format, 1234567890LL);
	format_integer(format, -1234567890LL);

	printf("\n\nFloat\n");
	printf("=====\n\n");
	format_float(format, 0.0);
	format_float(format, -0.0);
	format_float(format, 1234567890.0);
	format_float(format, -1234567890.0);
	format_float(format, 1234.56789);
	format_float(format, -1234.56789);
	format.DefaultFloatFormatParameters()->SetUseGrouping(true);
	format_float(format, 0.0);
	format_float(format, -0.0);
	format_float(format, 1234567890.0);
	format_float(format, -1234567890.0);
	format_float(format, 1234.56789);
	format_float(format, -1234.56789);
	return 0;
}

