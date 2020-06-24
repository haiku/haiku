/*
 * Copyright 2002, Ryan Fleet.
 * Copyright 2006-2007, Axel Dörfler, axeld@pinc-software.de.
 *
 * Distributed under the terms of the MIT license.
 */


#include <AppFileInfo.h>
#include <String.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern const char *__progname;

const char *kProgramName = __progname;


enum arg_needed {
	switch_needed, major_version, middle_version, minor_version,
	variety_version, internal_version, long_string, short_string
};

enum app_error {
	e_base = B_ERRORS_END,
	e_unknown, e_app_sys_switch, e_specify_version, e_major_version,
	e_middle_version, e_minor_version, e_variety_version, e_internal_version,
	e_expecting, e_long_string, e_short_string,
	e_parameter, e_app_twice, e_sys_twice
};

enum processing_mode { no_switch, app_switch, sys_switch };


static void
usage()
{
	fprintf(stdout, "Usage: %s filename\n", kProgramName);
	fprintf(stdout, "   [ -system <major> <middle> <minor>\n");
	fprintf(stdout, "       [ [ d | a | b | g | gm | f ] [ <internal> ] ]\n");
	fprintf(stdout, "       [ -short <shortVersionString> ]\n");
	fprintf(stdout, "       [ -long <longVersionString> ] ] # system info\n");
	fprintf(stdout, "   [ -app <major> <middle> <minor>\n");
	fprintf(stdout, "       [ [ d | a | b | g | gm | f ] [ <internal> ] ]\n");
	fprintf(stdout, "       [ -short <shortVersionString> ]\n");
	fprintf(stdout, "       [ -long <longVersionString> ] ] # application info\n");
}


static int
convertVariety(const char *str)
{
	if (!strcmp(str, "d") || !strcmp(str, "development"))
		return 0;
	if (!strcmp(str, "a") || !strcmp(str, "alpha"))
		return 1;
	if (!strcmp(str, "b") || !strcmp(str, "beta"))
		return 2;
	if (!strcmp(str, "g") || !strcmp(str, "gamma"))
		return 3;
	if (!strcmp(str, "gm") || !strcmp(str, "goldenmaster"))
		return 4;
	if (!strcmp(str, "f") || !strcmp(str, "final"))
		return 5;

	return -1;
}


static void
errorToString(BString& output, status_t error, const char *appName = NULL)
{
	switch (error) {
		case e_app_sys_switch:
			output = "-system or -app expected\n";
			break;
		case e_specify_version:
			output = "you did not specify any version\n";
			break;
		case e_major_version:
			output = "major version number error\n";
			break;
		case e_middle_version:
			output = "middle version number error\n";
			break;
		case e_minor_version:
			output = "minor version number error\n";
			break;
		case e_variety_version:
			output = "variety letter error\n";
			break;
		case e_internal_version:
			output = "internal version number error\n";
			break;
		case e_expecting:
			output = "expecting -short, -long, -app or -system\n";
			break;
		case e_long_string:
			output = "expecting long version string\n";
			break;
		case e_short_string:
			output = "expecting short version string\n";
			break;
		case e_parameter:
			output = "parameter error\n";
			break;
		case e_app_twice:
			output = "you cannot specify the app version twice\n";
			break;
		case e_sys_twice:
			output = "you cannot specify the system version twice\n";
			break;
		case e_unknown:
			output = "unknown internal error\n";
			break;

		default:
			output = strerror(error);

			if (appName != NULL) {
				output += ": ";
				output += appName;
			}
			break;
	}
}


static void
errorOut(status_t error, const char *appName = NULL, bool showUsage = true)
{
	BString output;
	errorToString(output, error, appName);

	fprintf(stderr, "%s: %s", kProgramName, output.String());

	if (showUsage)
		usage();

	exit(1);
}


static void
parse(bool &systemModified, bool &appModified, arg_needed &argNeeded,
	processing_mode &mode, version_info &systemVersion, version_info &appVersion,
	int argc, char *argv[])
{
	systemModified = false;
	appModified = false;
	mode = no_switch;
	argNeeded = switch_needed;

	for (int i = 2; i < argc; ++i) {
		version_info &version = mode == app_switch ? appVersion : systemVersion;

		switch (argNeeded) {
			case switch_needed:
				if (strcmp(argv[i], "-app") == 0) {
					if (mode == app_switch)
						errorOut(e_app_twice);
					if (appModified)
						errorOut(e_parameter);

					mode = app_switch;
					argNeeded = major_version;
					appModified = true;
				} else if (strcmp(argv[i], "-system") == 0) {
					if (mode == sys_switch)
						errorOut(e_sys_twice);
					if (systemModified)
						errorOut(e_parameter);

					mode = sys_switch;
					argNeeded = major_version;
					systemModified = true;
				} else if (strcmp(argv[i], "-long") == 0) {
					if (mode == no_switch)
						errorOut(e_app_sys_switch);

					argNeeded = long_string;
				} else if (strcmp(argv[i], "-short") == 0) {
					if (mode == no_switch)
						errorOut(e_app_sys_switch);

					argNeeded = short_string;
				} else if (mode == no_switch)
					errorOut(e_app_sys_switch);
				else if (strncmp(argv[i], "-", 1) == 0)
					errorOut(e_parameter);
				else
					errorOut(e_expecting);
				break;

			case major_version:
				if (isalpha(argv[i][0]))
					errorOut(e_major_version);

				version.major = atoi(argv[i]);
				argNeeded = middle_version;
				break;

			case middle_version:
				if (isalpha(argv[i][0]))
					errorOut(e_middle_version);

				version.middle = atoi(argv[i]);
				argNeeded = minor_version;
				break;

			case minor_version:
				if (isalpha(argv[i][0]))
					errorOut(e_minor_version);

				version.minor = atoi(argv[i]);

				if (i >= argc-1) {
					argNeeded = switch_needed;
					break;
				}

				argNeeded = variety_version;
				break;

			case variety_version:
			{
				if (!strncmp(argv[i], "-", 1)) {
					i--;
					argNeeded = switch_needed;
					break;
				}

				int variety = convertVariety(argv[i]);
				if (variety < 0)
					errorOut(e_variety_version);

				version.variety = variety;
				argNeeded = internal_version;
				break;
			}

			case internal_version:
				if (isalpha(argv[i][0]))
					errorOut(e_expecting);

				version.internal = atoi(argv[i]);
				argNeeded = switch_needed;
				break;

			case long_string:
				strcpy(version.long_info, argv[i]);
				argNeeded = switch_needed;
				break;

			case short_string:
				strcpy(version.short_info, argv[i]);
				argNeeded = switch_needed;
				break;
		}
	}

	if (mode == no_switch)
		errorOut(e_app_sys_switch);

	switch (argNeeded) {
		case major_version:
			errorOut(e_major_version);
			break;
		case middle_version:
			errorOut(e_middle_version);
			break;
		case minor_version:
			errorOut(e_minor_version);
			break;
		case variety_version:
			errorOut(e_variety_version);
			break;
		case internal_version:
			errorOut(e_internal_version);
			break;
		case long_string:
			errorOut(e_long_string);
			break;
		case short_string:
			errorOut(e_short_string);
			break;
		case switch_needed:
			// all is well
			break;
	}
}


int
main(int argc, char *argv[])
{
	if (argc < 3) {
		if (argc < 2)
			errorOut(e_app_sys_switch);

		errorOut(e_specify_version);
	}

	// reset version infos

	version_info systemVersion, appVersion;
	memset(&systemVersion, 0, sizeof(version_info));
	memset(&appVersion, 0, sizeof(version_info));

	// process arguments

	processing_mode mode;
	arg_needed argNeeded;
	bool systemModified, appModified;

	parse(systemModified, appModified, argNeeded, mode, systemVersion,
		appVersion, argc, argv);

	// write back changes

	BFile file;
	status_t status = file.SetTo(argv[1], B_READ_WRITE);
	if (status != B_OK)
		errorOut(status, argv[1], false);

	BAppFileInfo info;
	status = info.SetTo(&file);
	if (status != B_OK)
		errorOut(status, argv[1], false);

	if (systemModified ^ appModified) {
		// clear out other app info if not present - this works around a
		// bug in BeOS, see bug #681.
		version_kind kind = systemModified ? B_APP_VERSION_KIND : B_SYSTEM_VERSION_KIND;
		version_info clean;

		if (info.GetVersionInfo(&clean, kind) != B_OK) {
			memset(&clean, 0, sizeof(version_info));
			info.SetVersionInfo(&clean, kind);
		}
	}

	if (appModified) {
		status = info.SetVersionInfo(&appVersion, B_APP_VERSION_KIND);
		if (status < B_OK)
			errorOut(status, NULL, false);
	}

	if (systemModified) {
		status = info.SetVersionInfo(&systemVersion, B_SYSTEM_VERSION_KIND);
		if (status < B_OK)
			errorOut(status, NULL, false);
	}

	return 0;
}

