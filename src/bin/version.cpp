// Author: Ryan Fleet
// Created: 9th October 2002
// Modified: 14th October 2002

#include <cstdio>
#include <cstring>
#include <AppFileInfo.h>


void help(void)
{
	printf("usage: version [OPTION] FILENAME [FILENAME2, ...]\n");
	printf("Returns the version of a file.\n\n");
	printf("        -h, --help              this usage message\n");
	printf("        -l, --long              print long version information of FILENAME\n");
	printf("        -n, --numerical         print in numerical mode\n");
	printf("                                (Major miDdle miNor Variety Internal)\n");
	printf("        -s, --system            print system version instead of app version\n");
	printf("        --version               print version information for this command\n");
}


int getversion(const char *filename, version_kind kind, bool bLongFlag, bool bNumericalFlag)
{
	BFile file(filename, O_RDONLY);
	if(file.InitCheck() != B_OK)
	{
		printf("Couldn't get file info!\n");
		return 1;
	}
	
	BAppFileInfo info(&file);
	if(info.InitCheck() != B_OK)
	{
		printf("Couldn't get file info!\n");
		return 1;
	}
		
	version_info version;
	if(info.GetVersionInfo(&version, kind) != B_OK)
	{
		printf("Version unknown!\n");
		return 1;
	}
	
	if(true == bLongFlag)
	{
		printf("%s\n", version.long_info);
		return 0;
	}
	
	if(true == bNumericalFlag)
	{
		printf("%lu ", version.major);
		printf("%lu ", version.middle);
		printf("%lu ", version.minor);
		switch(version.variety)
		{
		case 0: printf("d "); break;
		case 1: printf("a "); break;
		case 2: printf("b "); break;
		case 3: printf("g "); break;
		case 4: printf("gm "); break;
		case 5: printf("f "); break;	
		};
		printf("%lu\n", version.internal);
		return 0;
	}
	
	printf("%s\n", version.short_info);
	
	return 0;
}


/* 
	strLessEqual(string1, string2)
	determines whether string1 contains at least one or more of the characters
	of string2 but none of which string2 doesn't contain. 
	
	true == ("hel" == "help"); true == ("help" == "help"); true == ("h" == "help");
	false == ("held" == "help"); false == ("helped" == "help"); 
*/
bool strLessEqual(const char *str1, const char *str2)
{
	char *ptr1 = const_cast<char*>(str1);
	char *ptr2 = const_cast<char*>(str2);
	
	while(*ptr1 != '\0')
	{	
		if(*ptr1 != *ptr2)
			return false;
		++ptr1;
		++ptr2;
	}
	return true;
}


int main(int argc, char *argv[])
{
	version_kind kind = B_APP_VERSION_KIND;
	bool bLongFlag = false;
	bool bNumericalFlag = false;
	int i;
	
	if(argc < 2)
		return 0;
	
	for(i = 1; i < argc; ++i)
	{
		if(strncmp(argv[i], "-", 1) == 0)
		{
			char *ptr = argv[i];
			++ptr;
			
			if(*ptr == '-')
			{
				bool lequal = false;
				++ptr;
				
				if(*ptr == 'h')
				{
					lequal = strLessEqual(ptr, "help");
					if(lequal == true)
					{
						help();
						return 0;
					}
				}
				
				else if(*ptr == 'l')
				{
					lequal = strLessEqual(ptr, "long");
					if(lequal == true)
						bLongFlag = true;
				}
				
				else if(*ptr == 'n')
				{
					lequal = strLessEqual(ptr, "numerical");
					if(lequal == true)
						bNumericalFlag = true;
				}
				
				else if(*ptr == 's')
				{
					lequal = strLessEqual(ptr, "system");
					if(lequal == true)
						kind = B_SYSTEM_VERSION_KIND;
				}
				
				else if(*ptr == 'v')
				{
					lequal = strLessEqual(ptr, "version");
					if(lequal == true)
					{
						getversion(argv[0], B_APP_VERSION_KIND, false, false);
						return 0;
					}
				}
				
				if(lequal == false)
					printf("%s unrecognized option `%s'\n", argv[0], argv[i]);
			} 
			
			else while(*ptr != '\0')
			{
				if(*ptr == 'h')
				{
					help();
					return 0;
				}
				else if(*ptr == 's')
					kind = B_SYSTEM_VERSION_KIND;
				else if(*ptr == 'l')
					bLongFlag = true;
				else if(*ptr == 'n')
					bNumericalFlag = true;
				else
					printf("%s: invalid option -- %c\n", argv[0], *ptr);
							
				++ptr;
			}
		}
	}
	
	int status = 0;
	int retval = 0;	
	for(i = 1; i < argc; ++i)
	{
		if(strncmp(argv[i], "-", 1) != 0)
		{
			status = getversion(argv[i], kind, bLongFlag, bNumericalFlag);
			if(status != 0)
				retval = 1;		
		}
	}

	return retval;
}
