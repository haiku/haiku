// Author: Ryan Fleet
// Created: 12th October 2002
// Modified: 14th October 2002


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <AppFileInfo.h>


enum eArgNeeded 
{ 
	switch_needed, major_version, middle_version, minor_version,
	variety_version, internal_version, long_string, short_string
};

enum eError
{
	e_unknown, e_app_sys_switch, e_specify_version, e_major_version,
	e_middle_version, e_minor_version, e_variety_version, e_internal_version,
	e_expecting, e_long_string, e_short_string, e_no_file, 
	e_parameter, e_app_twice, e_sys_twice
};

enum eMode { no_switch, app_switch, sys_switch };


void usage(const char *app)
{
	fprintf(stdout, "Usage: %s filename\n", app);
	fprintf(stdout, "   [ -system <major> <middle> <minor>\n");
	fprintf(stdout, "       [ [ d | a | b | g | gm | f ] [ <internal> ] ]\n");
	fprintf(stdout, "       [ -short <shortVersionString> ]\n");
	fprintf(stdout, "       [ -long <longVersionString> ] ] # system info\n");
	fprintf(stdout, "   [ -app <major> <middle> <minor>\n");
	fprintf(stdout, "       [ [ d | a | b | g | gm | f ] [ <internal> ] ]\n");
	fprintf(stdout, "       [ -short <shortVersionString> ]\n");
	fprintf(stdout, "       [ -long <longVersionString> ] ] # application info\n");
}


int convertVariety(const char *str)
{
	if(strcmp(str, "d") == 0)
		return 0;
	else if(strcmp(str, "a") == 0)
		return 1;
	else if(strcmp(str, "b") == 0)
		return 2;
	else if(strcmp(str, "g") == 0)
		return 3;
	else if(strcmp(str, "gm") == 0)
		return 4;
	else if(strcmp(str, "f") == 0)
		return 5;
	else
		return -1;
}


int errorOut(eError error, const char *thisApp, const char *appName = NULL)
{
	switch(error)
	{
	case e_app_sys_switch:
		fprintf(stderr, "%s: -system or -app expected\n", thisApp);
		break;
	case e_specify_version:
		fprintf(stderr, "%s: you did not specify any version\n", thisApp);
		break;
	case e_major_version:
		fprintf(stderr, "%s: major version number error\n", thisApp);
		break;
	case e_middle_version:
		fprintf(stderr, "%s: middle version number error\n", thisApp);
		break;
	case e_minor_version:
		fprintf(stderr, "%s: minor version number error\n", thisApp);
		break;
	case e_variety_version:
		fprintf(stderr, "%s: variety letter error\n", thisApp);
		break;
	case e_internal_version:
		fprintf(stderr, "%s: internal version number error\n", thisApp);
		break;
	case e_expecting:
		fprintf(stderr, "%s: expecting -short, -long, -app or -system\n", thisApp);
		break;
	case e_long_string: 
		fprintf(stderr, "%s: expecting long version string\n", thisApp);
		break;
	case e_short_string:
		fprintf(stderr, "%s: expecting short version string\n", thisApp);
		break;
	case e_no_file:
		fprintf(stderr, "%s: error No such file or directory opening file %s!\n", thisApp, appName);
		break;
	case e_parameter:
		fprintf(stderr, "%s: parameter error\n", thisApp);
		break;
	case e_app_twice:
		fprintf(stderr, "%s: you cannot specify the app version twice\n", thisApp);
		break;
	case e_sys_twice:
		fprintf(stderr, "%s: you cannot specify the system version twice\n", thisApp);
		break;
	case e_unknown:
		fprintf(stderr, "%s: unknown internal error\n", thisApp);
		break;
	};
	
	usage(thisApp);
	exit(1);
}


void parse(bool &bSysModified, bool &bAppModified, eArgNeeded &argNeeded, eMode &mode, version_info &sysInfo, version_info &appInfo, int argc, char *argv[])
{
	bSysModified = false;
	bAppModified = false;
	mode = no_switch;
	argNeeded = switch_needed;
	
	for(int i = 2; i < argc; ++i)
	{
		switch(argNeeded)
		{
		case switch_needed:
			{
				if(strcmp(argv[i], "-app") == 0)
				{
					if(mode == app_switch)
						errorOut(e_app_twice, argv[0]);
					if(true == bAppModified)
						errorOut(e_parameter, argv[0]);
					mode = app_switch;
					argNeeded = major_version;
					bAppModified = true;
				}
				else if(strcmp(argv[i], "-system") == 0)
				{
					if(mode == sys_switch)
						errorOut(e_sys_twice, argv[0]);
					if(true == bSysModified)
						errorOut(e_parameter, argv[0]);
					mode = sys_switch;
					argNeeded = major_version;
					bSysModified = true;
				}
				else if(strcmp(argv[i], "-long") == 0)
				{
					if(mode == no_switch)
						errorOut(e_app_sys_switch, argv[0]); 
						
					argNeeded = long_string;
				}
				else if(strcmp(argv[i], "-short") == 0)
				{
					if(mode == no_switch)
						errorOut(e_app_sys_switch, argv[0]); 
					argNeeded = short_string;
				}
				else if(mode == no_switch)
					errorOut(e_app_sys_switch, argv[0]);
				else if(strncmp(argv[i], "-", 1) == 0)
					errorOut(e_parameter, argv[0]);
				else
					errorOut(e_expecting, argv[0]); 
			} break;
			
		case major_version:
			{
				if(mode == app_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_major_version, argv[0]); 
					
					appInfo.major = atoi(argv[i]);
				}
				else if(mode == sys_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_major_version, argv[0]);
					
					sysInfo.major = atoi(argv[i]);
				}
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
			
				argNeeded = middle_version;	
			} break;
			
		case middle_version:
			{
				if(mode == app_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_middle_version, argv[0]); 
					
					appInfo.middle = atoi(argv[i]);
				}
				else if(mode == sys_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_middle_version, argv[0]);
					
					sysInfo.middle = atoi(argv[i]);	
				}
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				argNeeded = minor_version;
			} break;
			
		case minor_version:
			{
				if(mode == app_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_minor_version, argv[0]);
					
					appInfo.minor = atoi(argv[i]);	
				}
				else if(mode == sys_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_minor_version, argv[0]);
					
					sysInfo.minor = atoi(argv[i]);	
				}
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				
				if(i >= argc-1)
				{
					argNeeded = switch_needed;
					break;
				}		
				
				argNeeded = variety_version;
			} break;
		case variety_version:
			{
				if(strncmp(argv[i], "-", 1) == 0)
				{
					--i;
					argNeeded = switch_needed;
					break;
				}
			
				if(mode == app_switch)
				{
					int v = convertVariety(argv[i]);
					if(v < 0)
						errorOut(e_variety_version, argv[0]);
					appInfo.variety = v;
				}
				else if(mode == sys_switch)
				{	
					int v = convertVariety(argv[i]);
					if(v < 0)
						errorOut(e_variety_version, argv[0]);
					sysInfo.variety = v;
				}
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				argNeeded = internal_version;
			} break;
		case internal_version:
			{
				if(mode == app_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_expecting, argv[0]);
					
					appInfo.internal = atoi(argv[i]);	
				}
				else if(mode == sys_switch)
				{
					if(isalpha(argv[i][0]))
						errorOut(e_expecting, argv[0]); 
					
					sysInfo.internal = atoi(argv[i]);	
				}
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				argNeeded = switch_needed;
			} break;
		case long_string:
			{
				if(mode == app_switch)
					strcpy(appInfo.long_info, argv[i]);
				else if(mode == sys_switch)
					strcpy(sysInfo.long_info, argv[i]);
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				argNeeded = switch_needed;
			} break;
		case short_string:
			{
				if(mode == app_switch)
					strcpy(appInfo.short_info, argv[i]);
				else if(mode == sys_switch)
					strcpy(sysInfo.short_info, argv[i]);
				else
					errorOut(e_unknown, argv[0]); // (should never reach here)
				argNeeded = switch_needed;
			} break;
		};	
	}
	
	if(mode == no_switch)
		errorOut(e_app_sys_switch, argv[0]);
		
	switch(argNeeded)
	{
	case major_version:
		errorOut(e_major_version, argv[0]);
		break;
	case middle_version:
		errorOut(e_middle_version, argv[0]);
		break;
	case minor_version:
		errorOut(e_minor_version, argv[0]);
		break;
	case variety_version:
		errorOut(e_variety_version, argv[0]);
		break;
	case internal_version:
		errorOut(e_internal_version, argv[0]);
		break;
	case long_string:
		errorOut(e_long_string, argv[0]);
		break;
	case short_string:
		errorOut(e_short_string, argv[0]);
		break;
	case switch_needed:
		// all is well. continue
		break;
	};
}


int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		if(argc < 2)
			return errorOut(e_app_sys_switch, argv[0]);
		else
			return errorOut(e_specify_version, argv[0]);
	}
	
	eMode mode;
	eArgNeeded argNeeded;
	version_info sysInfo, appInfo;
	bool bSysModified, bAppModified;

	parse(bSysModified, bAppModified, argNeeded, mode, sysInfo, appInfo, argc, argv);
			
	BFile file(argv[1], B_WRITE_ONLY);
	if(file.InitCheck() != B_OK)
		errorOut(e_no_file, argv[0], argv[1]);
	 
	BAppFileInfo info(&file);
	if(info.InitCheck() != B_OK)
		errorOut(e_unknown, argv[0]);
		
	status_t status;
	if(bAppModified)
	{
		status = info.SetVersionInfo(&appInfo, B_APP_VERSION_KIND);
		if(status < 0)
			errorOut(e_unknown, argv[0]);
	}
	
	if(bSysModified)
	{
		status = info.SetVersionInfo(&sysInfo, B_SYSTEM_VERSION_KIND);	
		if(status < 0)
			errorOut(e_unknown, argv[0]);
	}	
	
	return 0;
}

