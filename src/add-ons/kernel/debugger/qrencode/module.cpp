/*
 * Copyright 2012, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */


#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "qrencode.h"
#include "qrspec.h"
}


extern "C" {
extern void abort_debugger_command();
}


static const char* kWebPostBaseURL = "http://mlotz.ch/q";

static char sStringBuffer[16 * 1024];
static char sEncodeBuffer[3 * 1024];
static int sBufferPosition = 0;
static int sQRCodeVersion = 19;
static QRecLevel sQRCodeLevel = QR_ECLEVEL_L;
static char sWebPostId[64];
static int sWebPostCounter = 0;


static bool
qrcode_bit(QRcode* qrCode, int x, int y)
{
	if (x >= qrCode->width || y >= qrCode->width)
		return false;

	return (qrCode->data[y * qrCode->width + x] & 0x01) == 1;
}


static void
move_to_and_clear_line(int line)
{
	kprintf(" \x1b[%dd\x1b[G\x1b[K", line + 1);
}


static bool
print_qrcode(QRcode* qrCode, bool waitForKey)
{
	move_to_and_clear_line(0);
	for (int y = 0; y < qrCode->width; y += 2) {
		move_to_and_clear_line(y / 2 + 1);
		kputs("  ");

		for (int x = 0; x < qrCode->width; x++) {
			bool upper = qrcode_bit(qrCode, x, y);
			bool lower = qrcode_bit(qrCode, x, y + 1);
			if (upper == lower)
				kputs(upper ? "\x11" : " ");
			else
				kputs(upper ? "\x12" : "\x13");
		}
	}

	move_to_and_clear_line(qrCode->width / 2 + 2);
	move_to_and_clear_line(qrCode->width / 2 + 3);

	if (waitForKey) {
		kputs("press q to abort or any other key to continue...\x1b[1B\x1b[G");
		if (kgetc() == 'q')
			return false;
	}

	return true;
}


static int
encode_url(const char* query, const char* data, int encodeLength,
	int inputLength)
{
	sEncodeBuffer[0] = 0;
	strlcat(sEncodeBuffer, kWebPostBaseURL, encodeLength + 1);
	strlcat(sEncodeBuffer, "?i=", encodeLength + 1);
	strlcat(sEncodeBuffer, sWebPostId, encodeLength + 1);
	strlcat(sEncodeBuffer, "&", encodeLength + 1);
	strlcat(sEncodeBuffer, query, encodeLength + 1);
	int position = strlcat(sEncodeBuffer, "=", encodeLength + 1);
	if (position > encodeLength)
		return -1;

	int copyCount = 0;
	while (inputLength > 0 && position < encodeLength) {
		char character = data[copyCount];
		if ((character >= 'a' && character <= 'z')
			|| (character >= 'A' && character <= 'Z')
			|| (character >= '0' && character <= '9')
			|| character == '.' || character == '-' || character == '_'
			|| character == '~') {
			sEncodeBuffer[position++] = character;
			sEncodeBuffer[position] = 0;
		} else {
			// Encode to a %xx escape.
			if (encodeLength - position < 3) {
				// Doesn't fit anymore, we're done.
				break;
			}

			char escaped[4];
			sprintf(escaped, "%%%.2x", character);
			position = strlcat(sEncodeBuffer, escaped, encodeLength + 1);
		}

		inputLength--;
		copyCount++;
	}

	return copyCount;
}


static int
qrencode(int argc, char* argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		kprintf("%s [<string>]\n", argv[0]);
		kprintf("If an argument is given, encodes that string as a QR code.\n"
			"Otherwise the current QR buffer is encoded as QR codes.\n"
			"When encoding from the QR buffer, the buffer is left intact.\n"
			"use qrclear to clear the QR buffer or qrflush to encode/clear.\n");
		return 1;
	}

	const char* source = NULL;
	if (argc < 2) {
		sStringBuffer[sBufferPosition] = 0;
		source = sStringBuffer;
	} else
		source = argv[1];

	int inputLength = strlen(source);
	int encodeLength = QRspec_getDataLength(sQRCodeVersion, sQRCodeLevel) - 3;
	while (inputLength > 0) {
		int copyCount = 0;
		if (sWebPostId[0] != 0) {
			copyCount = encode_url(sWebPostCounter++ == 0 ? "clear" : "d",
				source, encodeLength, inputLength);
			if (copyCount < 0) {
				kprintf("Failed to URL encode buffer.\n");
				QRcode_clearCache();
				return 1;
			}
		} else {
			copyCount = inputLength < encodeLength ? inputLength : encodeLength;
			memcpy(sEncodeBuffer, source, copyCount);
			sEncodeBuffer[copyCount] = 0;
		}

		QRcode* qrCode = QRcode_encodeString8bit(sEncodeBuffer, sQRCodeVersion,
			sQRCodeLevel);
		if (qrCode == NULL) {
			kprintf("Failed to encode buffer into qr code.\n");
			QRcode_clearCache();
			return 1;
		}

		source += copyCount;
		inputLength -= copyCount;

		bool doContinue = print_qrcode(qrCode, inputLength > 0);
		QRcode_free(qrCode);

		if (!doContinue) {
			QRcode_clearCache();
			abort_debugger_command();
				// Does not return.
		}
	}

	QRcode_clearCache();
	return 0;
}


static int
qrclear(int argc, char* argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		kprintf("Clears the current QR buffer.\n");
		return 0;
	}

	sBufferPosition = 0;
	sStringBuffer[0] = 0;
	return 0;
}


static int
qrflush(int argc, char* argv[])
{
	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		kprintf("Flushes the current QR buffer by encoding QR codes from\n"
			"the data and then clears the QR buffer.\n");
		return 1;
	}

	qrencode(0, NULL);
	qrclear(0, NULL);
	return 0;
}


static int
qrappend(int argc, char* argv[])
{
	if (argc < 2 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
		kprintf("%s <string>\n", argv[0]);
		kprintf("Appends the given string to the QR buffer. Can be used as\n"
			"the target of a pipe command to accumulate the output of other\n"
			"commands in the QR buffer.\n"
			"Note that this command will flush the QR buffer when it runs\n"
			"full to make room for the new string. This will cause QR codes\n"
			"to be generated while the append command still runs. As these\n"
			"implicit flushes only happen to make room in the buffer, the\n"
			"strings are afterwards appended aren't flushed, so make sure to\n"
			"execute the qrflush command to generate codes for these as\n"
			"well.\n");
		return 1;
	}

	const char* source = argv[1];
	int length = strlen(source) + 1;

	while (length > 0) {
		int copyCount = sizeof(sStringBuffer) - sBufferPosition - 1;
		if (copyCount == 0) {
			// The buffer is full, we need to flush it.
			qrflush(0, NULL);
		}

		if (length < copyCount)
			copyCount = length;

		memcpy(sStringBuffer + sBufferPosition, source, copyCount);
		sBufferPosition += copyCount;
		source += copyCount;
		length -= copyCount;
	}

	// Overwrite the 0 byte that was copied extra with a newline.
	if (sBufferPosition > 0)
		sStringBuffer[sBufferPosition - 1] = '\n';

	return 0;
}


static int
qrconfig(int argc, char* argv[])
{
	if (argc < 2 || (argc > 1 && strcmp(argv[1], "--help") == 0)) {
		kprintf("%s <version>\n", argv[0]);
		kprintf("Sets the QR version to be used. Valid versions range from 1\n"
			"to 40 where each version determines the size of the QR code.\n"
			"Version 1 is 21x21 in size and each version increments that size\n"
			"by 4x4 up to 177x177 for version 40.\n"
			"Bigger QR codes can hold more data, so ideally you use a version\n"
			"that makes the QR code as big as possible while still fitting on\n"
			"your screen.\n"
			"You can test the new config by running qrencode\n"
			"with a test string.\n"
			"Note that due to memory constraints in the kernel debugger, some\n"
			"of the higher versions may actually not work. The kernel\n"
			"debugger heap size can be adjusted using the KDEBUG_HEAP define\n"
			"in the kernel_debug_config.h header\n");
		return 1;
	}

	int newVersion = atoi(argv[1]);
	if (newVersion <= 0 || newVersion > 40) {
		kprintf("Invalid QR code version supplied, "
			"must be between 1 and 40.\n");
		return 1;
	}

	sQRCodeVersion = newVersion;
	return 0;
}


static int
qrwebpost(int argc, char* argv[])
{
	if (argc >= 3 && strcmp(argv[1], "start") == 0) {
		strlcpy(sWebPostId, argv[2], sizeof(sWebPostId));
		sWebPostCounter = 0;

		// Generate the clear code.
		const char* args[2] = { "", "yes" };
		qrencode(2, (char**)args);
	} else if (argc >= 2 && strcmp(argv[1], "stop") == 0) {
		sWebPostId[0] = 0;
	} else {
		kprintf("%s start <id>\n", argv[0]);
		kprintf("%s stop\n", argv[0]);
		kprintf("Causes the QR codes to be rendered as URLs that resolve to\n"
			"a service that allows appending the data of multiple QR codes\n"
			"for easier handling.\n"
			"An initial QR code is generated that invokes a clear operation\n"
			"to prepare the output file on the server.\n"
			"Note that there is no logic behind the service, if you and\n"
			"someone else use the same id at the same time, they will\n"
			"overwrite eachother. Therefore use a reasonably unique name.\n");
		return 1;
	}

	return 0;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			add_debugger_command("qrencode", qrencode,
				"encodes a string / the current QR buffer");
			add_debugger_command("qrappend", qrappend,
				"appends a string to the QR buffer");
			add_debugger_command("qrclear", qrclear,
				"clears the current QR buffer");
			add_debugger_command("qrflush", qrflush,
				"encodes the current QR buffer and clears it");
			add_debugger_command("qrconfig", qrconfig,
				"sets the QR code version to be used");
			add_debugger_command("qrwebpost", qrwebpost,
				"sets up URL encoding for QR codes");
			return B_OK;
		case B_MODULE_UNINIT:
			remove_debugger_command("qrencode", qrencode);
			remove_debugger_command("qrappend", qrappend);
			remove_debugger_command("qrclear", qrclear);
			remove_debugger_command("qrflush", qrflush);
			remove_debugger_command("qrconfig", qrconfig);
			remove_debugger_command("qrwebpost", qrwebpost);
			return B_OK;
	}

	return B_BAD_VALUE;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/qrencode/v1",
		0,
		&std_ops
	},
	NULL,
	NULL,
	NULL,
	NULL
};

module_info *modules[] = { 
	(module_info *)&sModuleInfo,
	NULL
};

