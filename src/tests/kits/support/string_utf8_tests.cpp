#include <stdio.h>
#include <stdlib.h>
#include <SupportDefs.h>
#include <String.h>
#include <InterfaceDefs.h>


inline void
expect(BString &string, const char *expect, size_t bytes, int32 chars)
{
	printf("expect: \"%s\" %lu %ld\n", expect, bytes, chars);
	printf("got:   \"%s\" %lu %ld\n", string.String(), string.Length(), string.CountChars());
	if (bytes != (size_t)string.Length()) {
		printf("expected byte length mismatch\n");
		exit(1);
	}

	if (chars != string.CountChars()) {
		printf("expected char count mismatch\n");
		exit(2);
	}

	if (memcmp(string.String(), expect, bytes) != 0) {
		printf("expected string mismatch\n");
		exit(3);
	}
}


int
main(int argc, char *argv[])
{
	printf("setting string to ü-ä-ö\n");
	BString string("ü-ä-ö");
	expect(string, "ü-ä-ö", 8, 5);

	printf("replacing ü and ö by ellipsis\n");
	string.ReplaceCharsSet("üö", B_UTF8_ELLIPSIS);
	expect(string, B_UTF8_ELLIPSIS "-ä-" B_UTF8_ELLIPSIS, 10, 5);

	printf("moving the last char (ellipsis) to a seperate string\n");
	BString ellipsis;
	string.MoveCharsInto(ellipsis, 4, 1);
	expect(string, B_UTF8_ELLIPSIS "-ä-", 7, 4);
	expect(ellipsis, B_UTF8_ELLIPSIS, 3, 1);

	printf("removing all - and ellipsis chars\n");
	string.RemoveCharsSet("-" B_UTF8_ELLIPSIS);
	expect(string, "ä", 2, 1);

	printf("reset the string to öäü" B_UTF8_ELLIPSIS "öäü\n");
	string.SetToChars("öäü" B_UTF8_ELLIPSIS "öäü", 5);
	expect(string, "öäü" B_UTF8_ELLIPSIS "ö", 11, 5);

	printf("truncating string to 4 characters\n");
	string.TruncateChars(4);
	expect(string, "öäü" B_UTF8_ELLIPSIS, 9, 4);

	printf("appending 2 chars out of \"öäü\"\n");
	string.AppendChars("öäü", 2);
	expect(string, "öäü" B_UTF8_ELLIPSIS "öä", 13, 6);

	printf("removing chars 1 through 4\n");
	string.RemoveChars(1, 3);
	expect(string, "ööä", 6, 3);

	printf("inserting 2 ellipsis out of 6 chars at offset 1\n");
	string.InsertChars("öäü" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "ä", 3, 2, 1);
	expect(string, "ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä", 12, 5);

	printf("prepending 3 out of 5 chars\n");
	string.PrependChars("ää+üü", 3);
	expect(string, "ää+ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä", 17, 8);

	printf("comparing first 5 chars which should succeed\n");
	const char *compare = "ää+ö" B_UTF8_ELLIPSIS "different";
	if (string.CompareChars(compare, 5) != 0) {
		printf("comparison failed\n");
		return 1;
	}

	printf("comparing first 6 chars which should fail\n");
	if (string.CompareChars(compare, 6) == 0) {
		printf("comparison succeeded\n");
		return 2;
	}

	printf("counting bytes of 3 chars from offset 2 expect 6\n");
	if (string.CountBytes(2, 3) != 6) {
		printf("got wrong byte count\n");
		return 3;
	}

	printf("all tests succeeded\n");
	return 0;
}
