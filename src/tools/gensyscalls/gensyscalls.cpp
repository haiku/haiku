// gensyscalls.cpp

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gensyscalls.h"
#include "gensyscalls_common.h"

extern "C" gensyscall_syscall_info *gensyscall_get_infos(int *count);

// usage
const char *kUsage =
"Usage: gensyscalls [ -c <calls> ] [ -d <dispatcher> ]\n"
"\n"
"The command generates a piece of assembly source file that defines the\n"
"actual syscalls and a piece of C source (cases of a switch statement) for"
"the kernel syscall dispatcher. Either file is to be included from a"
"respective skeleton source file.\n"
"\n"
"  <calls>                - Output: The assembly source file implementing the\n"
"                           actual syscalls."
"  <dispatcher>           - Output: The C source file to be included by the\n"
"                           syscall dispatcher source file.\n";

// print_usage
static
void
print_usage(bool error)
{
	fprintf((error ? stderr : stdout), kUsage);
}

// Main
class Main {
public:

	int Run(int argc, char **argv)
	{
		// parse arguments
		const char *syscallsFile = NULL;
		const char *dispatcherFile = NULL;
		for (int argi = 1; argi < argc; argi++) {
			string arg(argv[argi]);
			if (arg == "-h" || arg == "--help") {
				print_usage(false);
				return 0;
			} else if (arg == "-c") {
				if (argi + 1 >= argc) {
					print_usage(true);
					return 1;
				}
				syscallsFile = argv[++argi];
			} else if (arg == "-d") {
				if (argi + 1 >= argc) {
					print_usage(true);
					return 1;
				}
				dispatcherFile = argv[++argi];
			} else {
				print_usage(true);
				return 1;
			}
		}
		fSyscallInfos = gensyscall_get_infos(&fSyscallCount);
		if (!syscallsFile && !dispatcherFile) {
			printf("Found %d syscalls.\n", fSyscallCount);
			return 0;
		}
		// generate the output
		if (syscallsFile)
			_WriteSyscallsFile(syscallsFile);
		if (dispatcherFile)
			_WriteDispatcherFile(dispatcherFile);
		return 0;
	}

	void _WriteSyscallsFile(const char *filename)
	{
		// open the syscalls output file
		ofstream file(filename, ofstream::out | ofstream::trunc);
		if (!file.is_open())
			throw IOException(string("Failed to open `") + filename + "'.");
		// output the syscalls definitions
		for (int i = 0; i < fSyscallCount; i++) {
			const gensyscall_syscall_info &syscall = fSyscallInfos[i];
			int paramCount = syscall.parameter_count;
			int paramSize = 0;
			gensyscall_parameter_info* parameters = syscall.parameters;
			for (int k = 0; k < paramCount; k++) {
				int size = parameters[k].actual_size;
				paramSize += (size + 3) / 4 * 4;
			}
			file << "SYSCALL" << (paramSize / 4) << "("
				<< syscall.name << ", " << i << ")" << endl;
		}
	}

	void _WriteDispatcherFile(const char *filename)
	{
		// open the dispatcher output file
		ofstream file(filename, ofstream::out | ofstream::trunc);
		if (!file.is_open())
			throw IOException(string("Failed to open `") + filename + "'.");
		// output the case statements
		for (int i = 0; i < fSyscallCount; i++) {
			const gensyscall_syscall_info &syscall = fSyscallInfos[i];
			file << "case " << i << ":" << endl;
			file << "\t";
			if (string(syscall.return_type) != "void")
				file << "*call_ret = ";
			file << syscall.kernel_name << "(";
			int paramCount = syscall.parameter_count;
			if (paramCount > 0) {
				gensyscall_parameter_info* parameters = syscall.parameters;
				file << "*(" << _GetPointerType(parameters[0].type) << ")args";
				for (int k = 1; k < paramCount; k++) {
					file << ", *(" << _GetPointerType(parameters[k].type)
						<< ")((char*)args + " << parameters[k].offset << ")";
				}
			}
			file << ");" << endl;
			file << "\tbreak;" << endl;
		}
	}

	static string _GetPointerType(const char *type)
	{
		char *parenthesis = strchr(type, ')');
		if (!parenthesis)
			return string(type) + "*";
		// function pointer type
		return string(type, parenthesis - type) + "*" + parenthesis;
	}

private:
	const gensyscall_syscall_info	*fSyscallInfos;
	int								fSyscallCount;
};

// main
int
main(int argc, char **argv)
{
	try {
		return Main().Run(argc, argv);
	} catch (Exception &exception) {
		fprintf(stderr, "%s\n", exception.what());
		return 1;
	}
}
