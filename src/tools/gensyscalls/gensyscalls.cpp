// gensyscalls.cpp

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <inttypes.h>

#include "arch_config.h"

#include "gensyscalls.h"
#include "gensyscalls_common.h"

// We can't use SupportDefs.h to maintain portability
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


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

enum {
	PARAMETER_ALIGNMENT	= sizeof(FUNCTION_CALL_PARAMETER_ALIGNMENT_TYPE)
};

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
		_UpdateSyscallInfos();
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
			// XXX: Currently the SYSCALL macros support 4 byte aligned
			// parameters only. This has to change, of course.
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
				if (parameters[0].size < PARAMETER_ALIGNMENT) {
					file << "(" << parameters[0].type << ")*("
						 << "FUNCTION_CALL_PARAMETER_ALIGNMENT_TYPE"
						 << "*)args";
				} else {
					file << "*(" << _GetPointerType(parameters[0].type)
						<< ")args";
				}
				for (int k = 1; k < paramCount; k++) {
					if (parameters[k].size < PARAMETER_ALIGNMENT) {
						file << ", (" << parameters[k].type << ")*("
							<< "FUNCTION_CALL_PARAMETER_ALIGNMENT_TYPE"
							<< "*)((char*)args + " << parameters[k].offset
							<< ")";
					} else {
						file << ", *(" << _GetPointerType(parameters[k].type)
							<< ")((char*)args + " << parameters[k].offset
							<< ")";
					}
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

	void _UpdateSyscallInfos()
	{
		// Since getting the parameter offsets and actual sizes doesn't work
		// as it is now, we overwrite them with values computed using the
		// parameter alignment type.
		for (int i = 0; i < fSyscallCount; i++) {
			gensyscall_syscall_info &syscall = fSyscallInfos[i];
			int paramCount = syscall.parameter_count;
			gensyscall_parameter_info* parameters = syscall.parameters;
			int offset = 0;
			for (int k = 0; k < paramCount; k++) {
				if (parameters[k].size < PARAMETER_ALIGNMENT)
					parameters[k].actual_size = PARAMETER_ALIGNMENT;
				else
					parameters[k].actual_size = parameters[k].size;
				parameters[k].offset = offset;
				offset += parameters[k].actual_size;
			}
		}
	}

private:
	gensyscall_syscall_info	*fSyscallInfos;
	int						fSyscallCount;
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
