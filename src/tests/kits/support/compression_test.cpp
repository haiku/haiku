/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <File.h>

#include <ZlibCompressionAlgorithm.h>


extern const char* __progname;
const char* kCommandName = __progname;


enum CompressionType {
	ZlibCompression,
	GzipCompression,
};


static const char* kUsage =
	"Usage: %s <options> <input file> <output file>\n"
	"Compresses or decompresses (option -d) a file.\n"
	"\n"
	"Options:\n"
	"  -0 ... -9\n"
	"      Use compression level 0 ... 9. 0 means no, 9 best compression.\n"
	"      Defaults to 9.\n"
	"  -d, --decompress\n"
	"      Decompress the input file (default is compress).\n"
	"  -f <format>\n"
	"      Specify the compression format: \"zlib\" (default), or \"gzip\"\n"
	"  -h, --help\n"
	"      Print this usage info.\n"
	"  -i, --input-stream\n"
	"      Use the input stream API (default is output stream API).\n"
;


static void
print_usage_and_exit(bool error)
{
    fprintf(error ? stderr : stdout, kUsage, kCommandName);
    exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{
	int compressionLevel = B_ZLIB_COMPRESSION_DEFAULT;
	bool compress = true;
	bool useInputStream = true;
	CompressionType compressionType = ZlibCompression;

	while (true) {
		static struct option sLongOptions[] = {
			{ "decompress", no_argument, 0, 'd' },
			{ "help", no_argument, 0, 'h' },
			{ "input-stream", no_argument, 0, 'i' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+0123456789df:hi",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				compressionLevel = c - '0';
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'd':
				compress = false;
				break;

			case 'f':
				if (strcmp(optarg, "zlib") == 0) {
					compressionType = ZlibCompression;
				} else if (strcmp(optarg, "gzip") == 0) {
					compressionType = GzipCompression;
				} else {
					fprintf(stderr, "Error: Unsupported compression type "
						"\"%s\"\n", optarg);
					return 1;
				}
				break;

			case 'i':
				useInputStream = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are input and output file.
	if (optind + 2 != argc)
		print_usage_and_exit(true);

	const char* inputFilePath = argv[optind++];
	const char* outputFilePath = argv[optind++];

	// open input file
	BFile inputFile;
	status_t error = inputFile.SetTo(inputFilePath, B_READ_ONLY);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to open \"%s\": %s\n", inputFilePath,
			strerror(errno));
		return 1;
	}

	// open output file
	BFile outputFile;
	error = outputFile.SetTo(outputFilePath,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to open \"%s\": %s\n", outputFilePath,
			strerror(errno));
		return 1;
	}

	// create compression algorithm and parameters
	BCompressionAlgorithm* compressionAlgorithm;
	BCompressionParameters* compressionParameters;
	BDecompressionParameters* decompressionParameters;
	switch (compressionType) {
		case ZlibCompression:
		case GzipCompression:
		{
			compressionAlgorithm = new BZlibCompressionAlgorithm;
			BZlibCompressionParameters* zlibCompressionParameters
				= new BZlibCompressionParameters(compressionLevel);
			zlibCompressionParameters->SetGzipFormat(
				compressionType == GzipCompression);
			compressionParameters = zlibCompressionParameters;
			decompressionParameters = new BZlibDecompressionParameters;
			break;
		}
	}

	if (useInputStream) {
		// create input stream
		BDataIO* inputStream;
		if (compress) {
			error = compressionAlgorithm->CreateCompressingInputStream(
				&inputFile, compressionParameters, inputStream);
		} else {
			error = compressionAlgorithm->CreateDecompressingInputStream(
				&inputFile, decompressionParameters, inputStream);
		}

		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to create input stream: %s\n",
				strerror(error));
			return 1;
		}

		// processing loop
		for (;;) {
			uint8 buffer[64 * 1024];
			ssize_t bytesRead = inputStream->Read(buffer, sizeof(buffer));
			if (bytesRead < 0) {
				fprintf(stderr, "Error: Failed to read from input stream: %s\n",
					strerror(bytesRead));
				return 1;
			}
			if (bytesRead == 0)
				break;

			error = outputFile.WriteExactly(buffer, bytesRead);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to write to output file: %s\n",
					strerror(error));
				return 1;
			}
		}
	} else {
		// create output stream
		BDataIO* outputStream;
		if (compress) {
			error = compressionAlgorithm->CreateCompressingOutputStream(
				&outputFile, compressionParameters, outputStream);
		} else {
			error = compressionAlgorithm->CreateDecompressingOutputStream(
				&outputFile, decompressionParameters, outputStream);
		}

		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to create output stream: %s\n",
				strerror(error));
			return 1;
		}

		// processing loop
		for (;;) {
			uint8 buffer[64 * 1024];
			ssize_t bytesRead = inputFile.Read(buffer, sizeof(buffer));
			if (bytesRead < 0) {
				fprintf(stderr, "Error: Failed to read from input file: %s\n",
					strerror(bytesRead));
				return 1;
			}
			if (bytesRead == 0)
				break;

			error = outputStream->WriteExactly(buffer, bytesRead);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to write to output stream: %s\n",
					strerror(error));
				return 1;
			}
		}

		// flush the output stream
		error = outputStream->Flush();
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to flush output stream: %s\n",
				strerror(error));
			return 1;
		}
	}

	return 0;
}
