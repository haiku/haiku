/*
 * Copyright 2002-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */

#include <stdio.h>
#include <PrintJob.h>

#include "PicturePrinter.h"
#include "PrintJobReader.h"

void printArguments(const char* application) {
	printf("Usage: %s [--pictures] files\n", application);
	printf("Prints the contents of the specified spool files to stdout.\n"
		"Arguments:\n"
		"  --pictures  print contents of pictures\n");
}

int main(int argc, char* argv[]) {
	BApplication app("application/x-vnd.Haiku.dump-print-job");
	
	bool printPicture = false;
	bool hasFiles = false;
	
	for (int i = 1; i < argc; i ++) {
		const char* arg = argv[i];
		if (strcmp(arg, "--pictures") == 0) {
			printPicture = true;
			continue;
		}

		hasFiles = true;
		BFile jobFile(arg, B_READ_WRITE);
		if (jobFile.InitCheck() != B_OK) {
			fprintf(stderr, "Error opening file %s!\n", arg);
			continue;
		}			

		PrintJobReader reader(&jobFile);
		if (reader.InitCheck() != B_OK) {
			fprintf(stderr, "Error reading spool file %s!", arg); 
			continue;
		}
		
		printf("Spool file: %s\n", arg); 
		printf("Job settings message:\n");
		reader.JobSettings()->PrintToStream();
		
		int32 pages = reader.NumberOfPages();
		printf("Number of pages: %d\n", (int)pages);
		for (int page = 0; page < pages; page ++) {
			printf("Page: %d\n", page+1);
			PrintJobPage pjp;
			if (reader.GetPage(page, pjp) != B_OK) {
				fprintf(stderr, "Error reading page!\n");
				break;
			}
				
			BPicture picture;
			BPoint pos; 
			BRect rect;
			printf("Number of pictures: %ld\n", pjp.NumberOfPictures());
			while (pjp.NextPicture(picture, pos, rect) == B_OK) {
				printf("Picture position = (%f, %f) bounds = [l: %f t: %f r: %f b: %f]\n",
					pos.x, pos.y,
					rect.left, rect.top, rect.right, rect.bottom);

				if (printPicture) {
					printf("Picture:\n");
					PicturePrinter printer;
					printer.Iterate(&picture);
				}
			}
		}
	}
	
	if (!hasFiles)
		printArguments(argv[0]);
}
