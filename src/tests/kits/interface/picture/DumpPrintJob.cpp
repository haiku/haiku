/*

DumpPrintJob

Copyright (c) 2002 OpenBeOS. 

Author: 
	Michael Pfeiffer
	
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <stdio.h>
#include <PrintJob.h>

#include "PicturePrinter.h"
#include "PrintJobReader.h"

void print(char* s) {
	fprintf(stderr, s);
}

void print(char* argv[]) {
	fprintf(stderr, "%s: ", argv[0]);
}

status_t dump_page(BFile* jobFile) {
	int32 pictureCount;
	status_t rc = B_OK;
	jobFile->Read(&pictureCount, sizeof(pictureCount));
	printf("Pictures %ld\n", pictureCount);
	for (int i = 0; rc == B_OK && i < pictureCount; i ++) {
		BPoint p; BRect r;
		off_t v;
		jobFile->Read(&v, sizeof(v));
		printf("next picture %Lx %Lx %Lx\n", jobFile->Position(), v, v + jobFile->Position());
		jobFile->Seek(40, SEEK_CUR);

		jobFile->Read(&p, sizeof(p));
		jobFile->Read(&r, sizeof(r));
		BPicture picture;
		rc = picture.Unflatten(jobFile);
		if (rc == B_OK) {
			PicturePrinter printer;
			printf("Picture point (%f, %f) rect [l: %f t: %f r: %f b: %f]\n",
				p.x, p.y,
				r.left, r.top, r.right, r.bottom);
			// printer.Iterate(&picture);
			printf("==========================\n");
		} else {
			fprintf(stderr, "Error unflattening picture\n");
		}
	}
	return rc;
}

int main(int argc, char* argv[]) {
	BApplication app("application/x-vnd.obos.dump-print-job");
	for (int i = 1; i < argc; i ++) {
		BFile jobFile(argv[i], B_READ_WRITE);
		if (jobFile.InitCheck() == B_OK) {
			PrintJobReader reader(&jobFile);
			if (reader.InitCheck() == B_OK) {
					printf("%s: JobMessage\n", argv[0]);
					reader.JobSettings()->PrintToStream();
					int32 pages = reader.NumberOfPages();
					for (int page = 0; page < pages; page ++) {
						printf("Page %d\n", page+1);
						PrintJobPage pjp;
						if (reader.GetPage(page, pjp) == B_OK) {
							BPicture picture; BPoint p; BRect r;
							printf("Number of pictures %ld\n", pjp.NumberOfPictures());
							while (pjp.NextPicture(picture, p, r) == B_OK) {
								printf("Picture point (%f, %f) rect [l: %f t: %f r: %f b: %f]\n",
									p.x, p.y,
									r.left, r.top, r.right, r.bottom);
				
												PicturePrinter printer;
								printer.Iterate(&picture);
							}
						}
					}
			}
/*
			print_file_header header;
			if (jobFile.Read(&header, sizeof(header)) == sizeof(header)) {
				BMessage jobMsg;
				if (jobMsg.Unflatten(&jobFile) == B_OK) {
					printf("%s: JobMessage\n", argv[0]);
					jobMsg.PrintToStream();
					int32 pages = header.page_count;
					for (int page = 0; page < pages; page ++) {
						printf("Page %d offset %lx\n", page+1, jobFile.Position());
						if (dump_page(&jobFile) != B_OK) break; 
					}
				} else {
					print(argv); print("Could not unflatten job message\n");
				}
			} else {
				print(argv); print("Could not read print_job_header\n");
			}
	*/
		} else {
			print(argv); print("Error opening file!\n");
		}
	}
}
