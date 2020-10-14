/*
 * Copyright 2004-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <list>
#include <stack>
#include <string>
#include <string.h>
#include <vector>

#include "gensyscalls_common.h"

#include "arch_gensyscalls.h"
	// for the alignment type macros (only for the type names)


// macro trickery to create a string literal
#define MAKE_STRING(x)	#x
#define EVAL_MACRO(macro, x) macro(x)


const char* kUsage =
	"Usage: gensyscallinfos <header> <syscall infos> <syscall types sizes>\n"
	"\n"
	"Given the (preprocessed) header file that defines the syscall prototypes "
		"the\n"
	"command generates a source file consisting of syscall infos, which is "
		"needed\n"
	"to build gensyscalls, which in turn generates the assembly syscall\n"
	"definitions and code for the kernelland syscall dispatcher.\n"
	"\n"
	"  <header>               - Input: The preprocessed header file with the\n"
	"                           syscall prototypes.\n"
	"  <syscall infos>        - Output: The syscall infos source file needed "
		"to\n"
	"                           build gensyscalls.\n"
	"  <syscall types sizes>  - Output: A source file that will by another "
		"build\n"
	"                           step turned into a header file included by\n"
	"                           <syscall infos>.\n";


static void
print_usage(bool error)
{
	fputs(kUsage, (error ? stderr : stdout));
}


struct Type {
	Type(const char* type) : type(type) {}
	Type(const string& type) : type(type) {}

	string type;
};


struct NamedType : Type {
	NamedType(const char* type, const char* name)
		: Type(type), name(name) {}
	NamedType(const string& type, const string& name)
		: Type(type), name(name) {}

	string name;
};


class Function {
public:
	Function() : fReturnType("") {}

	void SetName(const string& name)
	{
		fName = name;
	}

	const string& GetName() const
	{
		return fName;
	}

	void AddParameter(const NamedType& type)
	{
		fParameters.push_back(type);
	}

	int CountParameters() const
	{
		return fParameters.size();
	}

	const NamedType& ParameterAt(int index) const
	{
		return fParameters[index];
	}

	void SetReturnType(const Type& type)
	{
		fReturnType = type;
	}

	const Type& GetReturnType() const
	{
		return fReturnType;
	}

protected:
	string				fName;
	vector<NamedType>	fParameters;
	Type				fReturnType;
};


class Syscall : public Function {
public:
	string GetKernelName() const
	{
		int baseIndex = 0;
		if (fName.find("_kern_") == 0)
			baseIndex = strlen("_kern_");
		else if (fName.find("sys_") == 0)
			baseIndex = strlen("sys_");
		string kernelName("_user_");
		kernelName.append(string(fName, baseIndex));
		return kernelName;
	}
};


class Tokenizer {
public:
	Tokenizer(istream& input)
		: fInput(input),
		  fHasCurrent(false)
	{
	}

	string GetCurrentToken()
	{
		if (!fHasCurrent)
			throw Exception("GetCurrentToken(): No current token!");
		return fTokens.front();
	}

	string GetNextToken()
	{
		return GetNextToken(NULL);
	}

	string GetNextToken(stack<string>& skippedTokens)
	{
		return GetNextToken(&skippedTokens);
	}

	string GetNextToken(stack<string>* skippedTokens)
	{
		if (fHasCurrent) {
			fTokens.pop_front();
			fHasCurrent = false;
		}
		while (fTokens.empty())
			_ReadLine();
		fHasCurrent = true;
		if (skippedTokens)
			skippedTokens->push(fTokens.front());
		return fTokens.front();
	}

	void ExpectToken(const string& expectedToken)
	{
		string token = GetCurrentToken();
		if (expectedToken != token) {
			throw ParseException(string("Unexpected token `") + token
				+ "'. Expected was `" + expectedToken + "'.");
		}
	}

	void ExpectNextToken(const string& expectedToken)
	{
		GetNextToken();
		ExpectToken(expectedToken);
	}

	bool CheckToken(const string& expectedToken)
	{
		string token = GetCurrentToken();
		return (expectedToken == token);
	}

	bool CheckNextToken(const string& expectedToken)
	{
		GetNextToken();
		bool result = CheckToken(expectedToken);
		if (!result)
			PutCurrentToken();
		return result;
	}

	void PutCurrentToken()
	{
		if (!fHasCurrent)
			throw Exception("GetCurrentToken(): No current token!");
		fHasCurrent = false;
	}

	void PutToken(string token)
	{
		if (fHasCurrent) {
			fTokens.pop_front();
			fHasCurrent = false;
		}
		fTokens.push_front(token);
	}

	void PutTokens(stack<string>& tokens)
	{
		if (fHasCurrent) {
			fTokens.pop_front();
			fHasCurrent = false;
		}
		while (!tokens.empty()) {
			fTokens.push_front(tokens.top());
			tokens.pop();
		}
	}

private:
	void _ReadLine()
	{
		// read the line
		char buffer[10240];
		if (!fInput.getline(buffer, sizeof(buffer)))
			throw EOFException("Unexpected end of input.");
		// parse it
		vector<string> line;
		int len = strlen(buffer);
		int tokenStart = 0;
		for (int i = 0; i < len; i++) {
			char c = buffer[i];
			if (isspace(c)) {
				if (tokenStart < i)
					line.push_back(string(buffer + tokenStart, buffer + i));
				tokenStart = i + 1;
				continue;
			}
			switch (buffer[i]) {
				case '#':
				case '(':
				case ')':
				case '*':
				case '&':
				case '[':
				case ']':
				case ';':
				case ',':
					if (tokenStart < i) {
						line.push_back(string(buffer + tokenStart,
												 buffer + i));
					}
					line.push_back(string(buffer + i, buffer + i + 1));
					tokenStart = i + 1;
					break;
			}
		}
		if (tokenStart < len)
			line.push_back(string(buffer + tokenStart, buffer + len));
		// drop the line, if it starts with "# <number>", as those are
		// directions from the pre-processor to the compiler
		if (line.size() >= 2) {
			if (line[0] == "#" && isdigit(line[1][0]))
				return;
		}
		for (int i = 0; i < (int)line.size(); i++)
			fTokens.push_back(line[i]);
	}

private:
	istream&		fInput;
	list<string>	fTokens;
	bool			fHasCurrent;
};


class Main {
public:

	int Run(int argc, char** argv)
	{
		// parse parameters
		if (argc >= 2
			&& (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
			print_usage(false);
			return 0;
		}
		if (argc != 4) {
			print_usage(true);
			return 1;
		}
		_ParseSyscalls(argv[1]);
		_WriteSyscallInfoFile(argv[2]);
		_WriteSyscallTypeSizes(argv[3]);
		return 0;
	}

private:
	void _ParseSyscalls(const char* filename)
	{
		// open the input file
		ifstream file(filename, ifstream::in);
		if (!file.is_open())
			throw IOException(string("Failed to open `") + filename + "'.");
		// parse the syscalls
		Tokenizer tokenizer(file);
		// find "#pragma syscalls begin"
		while (true) {
			while (tokenizer.GetNextToken() != "#");
			stack<string> skippedTokens;
			if (tokenizer.GetNextToken(skippedTokens) == "pragma"
				&& tokenizer.GetNextToken(skippedTokens) == "syscalls"
				&& tokenizer.GetNextToken(skippedTokens) == "begin") {
				break;
			}
			tokenizer.PutTokens(skippedTokens);
		}
		// parse the functions
		while (!tokenizer.CheckNextToken("#")) {
			Syscall syscall;
			_ParseSyscall(tokenizer, syscall);
			fSyscalls.push_back(syscall);
		}
		// expect "pragma syscalls end"
		tokenizer.ExpectNextToken("pragma");
		tokenizer.ExpectNextToken("syscalls");
		tokenizer.ExpectNextToken("end");
	}

	void _WriteSyscallInfoFile(const char* filename)
	{
		// open the syscall info file
		ofstream file(filename, ofstream::out | ofstream::trunc);
		if (!file.is_open())
			throw IOException(string("Failed to open `") + filename + "'.");

		// write preamble
		file << "#include \"gensyscalls.h\"" << endl;
		file << "#include \"syscall_types_sizes.h\"" << endl;
		file << endl;

		file << "const char* const kReturnTypeAlignmentType = \""
			EVAL_MACRO(MAKE_STRING, SYSCALL_RETURN_TYPE_ALIGNMENT_TYPE)
			<< "\";" << endl;
		file << "const char* const kParameterAlignmentType = \""
			EVAL_MACRO(MAKE_STRING, SYSCALL_PARAMETER_ALIGNMENT_TYPE)
			<< "\";" << endl;
		file << "const int kReturnTypeAlignmentSize = "
			"SYSCALL_RETURN_TYPE_ALIGNMENT_SIZE;" << endl;
		file << "const int kParameterAlignmentSize = "
			"SYSCALL_PARAMETER_ALIGNMENT_SIZE;" << endl;
		file << endl;

		file << "SyscallVector* create_syscall_vector() {" << endl;
		file << "\tSyscallVector* syscallVector = SyscallVector::Create();"
			<< endl;
		file << "\tSyscall* syscall;" << endl;

		// syscalls
		for (int i = 0; i < (int)fSyscalls.size(); i++) {
			const Syscall& syscall = fSyscalls[i];

			// syscall = syscallVector->CreateSyscall("syscallName",
			//	"syscallKernelName");
			file << "\tsyscall = syscallVector->CreateSyscall(\""
				<< syscall.GetName() << "\", \""
				<< syscall.GetKernelName() << "\");" << endl;

			const Type& returnType = syscall.GetReturnType();

			// syscall->SetReturnType<(SYSCALL_RETURN_TYPE_SIZE_<i>,
			//		"returnType");
			file << "\tsyscall->SetReturnType("
				<< "SYSCALL_RETURN_TYPE_SIZE_" << i << ", \""
				<< returnType.type << "\");" << endl;

			// parameters
			int paramCount = syscall.CountParameters();
			for (int k = 0; k < paramCount; k++) {
				const NamedType& param = syscall.ParameterAt(k);
				// syscall->AddParameter(SYSCALL_PARAMETER_SIZE_<i>_<k>,
				//		"parameterTypeName", "parameterName");
				file << "\tsyscall->AddParameter("
					<< "SYSCALL_PARAMETER_SIZE_" << i << "_" << k
					<< ", \"" << param.type << "\", \"" << param.name << "\");"
					<< endl;
			}
			file << endl;
		}

		// postamble
		file << "\treturn syscallVector;" << endl;
		file << "}" << endl;
	}

	void _WriteSyscallTypeSizes(const char* filename)
	{
		// open the syscall info file
		ofstream file(filename, ofstream::out | ofstream::trunc);
		if (!file.is_open())
			throw IOException(string("Failed to open `") + filename + "'.");

		// write preamble
		file << "#include <computed_asm_macros.h>" << endl;
		file << "#include <syscalls.h>" << endl;
		file << endl;
		file << "#include \"arch_gensyscalls.h\"" << endl;
		file << endl;
		file << "void dummy() {" << endl;

		file << "DEFINE_COMPUTED_ASM_MACRO(SYSCALL_RETURN_TYPE_ALIGNMENT_SIZE, "
			"sizeof(SYSCALL_RETURN_TYPE_ALIGNMENT_TYPE));" << endl;
  		file << "DEFINE_COMPUTED_ASM_MACRO(SYSCALL_PARAMETER_ALIGNMENT_SIZE, "
			"sizeof(SYSCALL_PARAMETER_ALIGNMENT_TYPE));" << endl;
		file << endl;

		// syscalls
		for (int i = 0; i < (int)fSyscalls.size(); i++) {
			const Syscall& syscall = fSyscalls[i];
			const Type& returnType = syscall.GetReturnType();

			if (returnType.type == "void") {
				file << "DEFINE_COMPUTED_ASM_MACRO(SYSCALL_RETURN_TYPE_SIZE_"
					<< i << ", 0);" << endl;
			} else {
				file << "DEFINE_COMPUTED_ASM_MACRO(SYSCALL_RETURN_TYPE_SIZE_"
					<< i << ", sizeof(" << returnType.type << "));" << endl;
			}

			// parameters
			int paramCount = syscall.CountParameters();
			for (int k = 0; k < paramCount; k++) {
				const NamedType& param = syscall.ParameterAt(k);
				file << "DEFINE_COMPUTED_ASM_MACRO(SYSCALL_PARAMETER_SIZE_" << i
					<< "_" << k << ", sizeof(" << param.type << "));" << endl;
			}
			file << endl;
		}

		// postamble
		file << "}" << endl;
	}

	void _ParseSyscall(Tokenizer& tokenizer, Syscall& syscall)
	{
		// get return type and function name
		vector<string> returnType;
		while (tokenizer.GetNextToken() != "(") {
			string token = tokenizer.GetCurrentToken();
			// strip leading "extern"
			if (!returnType.empty() || token != "extern")
				returnType.push_back(token);
		}
		if (returnType.size() < 2) {
			throw ParseException("Error while parsing function "
				"return type.");
		}
		syscall.SetName(returnType[returnType.size() - 1]);
		returnType.pop_back();
		string returnTypeString(returnType[0]);
		for (int i = 1; i < (int)returnType.size(); i++) {
			returnTypeString += " ";
			returnTypeString += returnType[i];
		}
		syscall.SetReturnType(returnTypeString);
		// get arguments
		if (!tokenizer.CheckNextToken(")")) {
			_ParseParameter(tokenizer, syscall);
			while (!tokenizer.CheckNextToken(")")) {
				tokenizer.ExpectNextToken(",");
				_ParseParameter(tokenizer, syscall);
			}
		}
		tokenizer.ExpectNextToken(";");
	}

	void _ParseParameter(Tokenizer& tokenizer, Syscall& syscall)
	{
		vector<string> type;
		while (tokenizer.GetNextToken() != ")"
			&& tokenizer.GetCurrentToken() != ",") {
			string token = tokenizer.GetCurrentToken();
			type.push_back(token);
			if (token == "(") {
				// This must be a function pointer. We deal with that in a
				// separate method.
				_ParseFunctionPointerParameter(tokenizer, syscall, type);
				return;
			}
		}
		tokenizer.PutCurrentToken();
		if (type.size() < 2) {
			if (type.size() == 1 && type[0] == "void") {
				// that's OK
				return;
			}
			throw ParseException("Error while parsing function parameter.");
		}

		// last component is the parameter name
		string typeName = type.back();
		type.pop_back();

		string typeString(type[0]);
		for (int i = 1; i < (int)type.size(); i++) {
			typeString += " ";
			typeString += type[i];
		}
		syscall.AddParameter(NamedType(typeString, typeName));
	}

	void _ParseFunctionPointerParameter(Tokenizer& tokenizer, Syscall& syscall,
		vector<string>& type)
	{
		// When this method is entered, the return type and the opening
		// parenthesis must already be parse and stored in the supplied type
		// vector.
		if (type.size() < 2) {
			throw ParseException("Error parsing function parameter. "
				"No return type.");
		}
		// read all the "*"s there are
		while (tokenizer.CheckNextToken("*"))
			type.push_back("*");
		// now comes the parameter name, if specified -- skip it
		string typeName;
		if (!tokenizer.CheckNextToken(")")) {
			typeName = tokenizer.GetNextToken();
			tokenizer.ExpectNextToken(")");
		} else {
			throw ParseException("Error while parsing function parameter. "
				"No parameter name.");
		}
		type.push_back(")");
		// the function parameters
		tokenizer.ExpectNextToken("(");
		type.push_back("(");
		while (!tokenizer.CheckNextToken(")"))
			type.push_back(tokenizer.GetNextToken());
		type.push_back(")");
		// compose the type string and add it to the syscall parameters
		string typeString(type[0]);
		for (int i = 1; i < (int)type.size(); i++) {
			typeString += " ";
			typeString += type[i];
		}
		syscall.AddParameter(NamedType(typeString, typeName));
	}

private:
	vector<Syscall>	fSyscalls;
};


int
main(int argc, char** argv)
{
	try {
		return Main().Run(argc, argv);
	} catch (const Exception& exception) {
		fprintf(stderr, "%s\n", exception.what());
		return 1;
	}
}
