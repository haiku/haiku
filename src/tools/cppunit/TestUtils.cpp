// TestUtils.cpp

#include <TestUtils.h>
#include <TestShell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using std::cout;
using std::dec;
using std::endl;
using std::hex;
using std::string;

_EXPORT
status_t DecodeResult(status_t result) {
	if (!BTestShell::GlobalBeVerbose())
		return result;

	string str;
	switch (result) {

		case B_OK:
			str = "B_OK";
			break;

		case B_ERROR:
			str = "B_ERROR";
			break;


		// Storage Kit Errors
		case B_FILE_ERROR:
			str = "B_FILE_ERROR";
			break;

		case B_FILE_NOT_FOUND:
			str = "B_FILE_NOT_FOUND";
			break;

		case B_FILE_EXISTS:
			str = "B_FILE_EXISTS";
			break;

		case B_ENTRY_NOT_FOUND:
			str = "B_ENTRY_NOT_FOUND";
			break;

		case B_NAME_TOO_LONG:
			str = "B_NAME_TOO_LONG";
			break;

		case B_DIRECTORY_NOT_EMPTY:
			str = "B_DIRECTORY_NOT_EMPTY";
			break;

		case B_DEVICE_FULL:
			str = "B_DEVICE_FULL";
			break;

		case B_READ_ONLY_DEVICE:
			str = "B_READ_ONLY_DEVICE";
			break;

		case B_IS_A_DIRECTORY:
			str = "B_IS_A_DIRECTORY";
			break;

		case B_NO_MORE_FDS:
			str = "B_NO_MORE_FDS";
			break;

		case B_CROSS_DEVICE_LINK:
			str = "B_CROSS_DEVICE_LINK";
			break;

		case B_LINK_LIMIT:
			str = "B_LINK_LIMIT";
			break;

		case B_BUSTED_PIPE:
			str = "B_BUSTED_PIPE";
			break;

		case B_UNSUPPORTED:
			str = "B_UNSUPPORTED";
			break;

		case B_PARTITION_TOO_SMALL:
			str = "B_PARTITION_TOO_SMALL";
			break;

		case B_BAD_MIME_SNIFFER_RULE:
			str = "B_BAD_MIME_SNIFFER_RULE";
			break;

		// General Errors
		case B_NO_MEMORY:
			str = "B_NO_MEMORY";
			break;

		case B_IO_ERROR:
			str = "B_IO_ERROR";
			break;

		case B_PERMISSION_DENIED:
			str = "B_PERMISSION_DENIED";
			break;

		case B_BAD_INDEX:
			str = "B_BAD_INDEX";
			break;

		case B_BAD_TYPE:
			str = "B_BAD_TYPE";
			break;

		case B_BAD_VALUE:
			str = "B_BAD_VALUE";
			break;

		case B_MISMATCHED_VALUES:
			str = "B_MISMATCHED_VALUES";
			break;

		case B_NAME_NOT_FOUND:
			str = "B_NAME_NOT_FOUND";
			break;

		case B_NAME_IN_USE:
			str = "B_NAME_IN_USE";
			break;

		case B_TIMED_OUT:
			str = "B_TIMED_OUT";
			break;

		case B_INTERRUPTED:
			str = "B_INTERRUPTED";
			break;

		case B_WOULD_BLOCK:
			str = "B_WOULD_BLOCK";
			break;

		case B_CANCELED:
			str = "B_CANCELED";
			break;

		case B_NO_INIT:
			str = "B_NO_INIT";
			break;

		case B_BUSY:
			str = "B_BUSY";
			break;

		case B_NOT_ALLOWED:
			str = "B_NOT_ALLOWED";
			break;


		// Kernel Errors
		case B_BAD_ADDRESS:
			str = "B_BAD_ADDRESS";
			break;

		case B_BAD_TEAM_ID:
			str = "B_BAD_TEAM_ID";
			break;

		// OS Errors
		case B_BAD_PORT_ID:
			str = "B_BAD_PORT_ID";
			break;

		// Anything Else
		default:
			str = "??????????";
			break;

	}

	cout << endl << "DecodeResult() -- " "0x" << hex << result << " (" << dec << result << ") == " << str << endl;

	return result;
}

_EXPORT
string IntToStr(int i) {
	char num[32];
	sprintf(num, "%d", i);
	return string(num);
}

_EXPORT
void ExecCommand(const char *command) {
	if (command)
		system(command);
}

_EXPORT
void ExecCommand(const char *command, const char *parameter) {
	if (command && parameter) {
		char *cmdLine = new char[strlen(command) + strlen(parameter) + 1];
		strcpy(cmdLine, command);
		strcat(cmdLine, parameter);
		system(cmdLine);
		delete[] cmdLine;
	}
}

_EXPORT
void ExecCommand(const char *command, const char *parameter1,
							const char *parameter2) {
	if (command && parameter1 && parameter2) {
		char *cmdLine = new char[strlen(command) + strlen(parameter1)
								 + 1 + strlen(parameter2) + 1];
		strcpy(cmdLine, command);
		strcat(cmdLine, parameter1);
		strcat(cmdLine, " ");
		strcat(cmdLine, parameter2);
		system(cmdLine);
		delete[] cmdLine;
	}
}
