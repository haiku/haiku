// TestUtils.cpp

#include <TestUtils.h>
#include <TestShell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>


_EXPORT
std::string IntToStr(int i) {
	char num[32];
	sprintf(num, "%d", i);
	return std::string(num);
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
