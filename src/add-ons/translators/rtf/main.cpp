/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <Application.h>
#include <Catalog.h>
#include <FileIO.h>
#include <TranslatorRoster.h>

#include "TranslatorWindow.h"

#include "convert.h"
#include "RTF.h"
#include "RTFTranslator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "main"


int
main(int argc, char** argv)
{
	if (argc > 1) {
		// Convert input files to plain text directly
		BFileIO output(stdout);
		int result = 0;

		for (int i = 1; i < argc; i++) {
			BFile input;
			status_t status = input.SetTo(argv[i], B_READ_ONLY);
			if (status != B_OK) {
				syslog(LOG_ERR,
						"RTFTranslator:Could not open file \"%s\": %s\n",
											argv[i], strerror(status));
				result = 1;
				continue;
			}

			RTF::Parser parser(input);
			RTF::Header header;

			status = parser.Parse(header);
			if (status != B_OK) {
				syslog(LOG_ERR,
						"RTFTranslator:Could not convert file \"%s\": %s\n", 
											argv[i], strerror(status));
				result = 1;
				continue;
			}

			convert_to_plain_text(header, output);
		}

		return result;
	}

	BApplication app("application/x-vnd.Haiku-RTFTranslator");

	status_t result;
	result = LaunchTranslatorWindow(new RTFTranslator, 
		B_TRANSLATE("RTF Settings"), BRect(0, 0, 225, 175));
	if (result != B_OK)
		return 1;

	app.Run();
	return 0;
}

