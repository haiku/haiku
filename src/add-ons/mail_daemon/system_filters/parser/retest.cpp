/* To compile this test program, use this command line:
g++ -I../../include/numail -I../../include/public/ -I../../include/support/ -lbe -lmail -o Test -Wall retest.cpp
Then run Test with the subjects data file as input, or manually type in entries.
*/
#include <stdio.h>
#include <String.h>
#include <InterfaceDefs.h>

#include <mail_util.h>

int main(int argc, char** argv)
{
	BString string;
	char buf[1024];
	while (gets(buf))
	{
		string = buf;
		Zoidberg::Mail::SubjectToThread(string);
		printf ("Input:  \"%s\"\nOutput: \"%s\"\n\n", buf, string.String());
	}
	return 0;
}
