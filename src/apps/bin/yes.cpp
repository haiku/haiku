//	yes - for OpenBeOS
//
//	authors, in order of contribution:
//	jonas.sundstrom@kirilla.com
//

#include <stdio.h>
#include <string.h>
#include <String.h>

void	PrintUsageInfo	(void);

int main(int32 argc, char **argv)
{
	if (argc > 1)	// possible --help
	{
		BString option	=	argv[1];
		option.ToLower();
		
		if (option == "--help")
		{
			PrintUsageInfo();
			return (0);
		}
	}

	while (1)		// loop until interrupted
	{
		if (argc < 2)		// no STRING
		{
			printf("y\n");
		}
		else				// STRING(s)
		{
			for (int i = 1;  i < argc;  i++)
			{
				printf("%s ", argv[i]);
			}
			printf("\n");
		}
	}

	return (0);
}

void PrintUsageInfo (void)
{
	printf ("use: yes [STRING] ...\n"
			"Repeatedly output a line with all specified STRING(s), or `y'.\n");
}

