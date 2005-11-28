// gensyscallinfos.cpp
//
// Copyright (c) 2004, Ingo Weinhold <bonefish@cs.tu-berlin.de>
// All rights reserved. Distributed under the terms of the OpenBeOS License.

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <list>
#include <stack>
#include <string>
#include <vector>

#include "gensyscalls_common.h"

// usage
const char *kUsage =
"Usage: gensyscallinfos <header> <syscall infos>\n"
"\n"
"Given the (preprocessed) header file that defines the syscall prototypes the\n"
"command generates a source file consisting of syscall infos, which is needed\n"
"to build gensyscalls, which in turn generates the assembly syscall\n"
"definitions and code for the kernelland syscall dispatcher.\n"
"\n"
"  <header>               - Input: The preprocessed header file with the\n"
"                           syscall prototypes.\n"
"  <syscall infos>        - Output: The syscall infos source file needed to\n"
"                           build gensyscalls.";

// print_usage
static
void
print_usage(bool error)
{
	fprintf((error ? stderr : stdout), kUsage);
}

// Type
struct Type {
	Type(const char *type) : type(type) {}
	Type(const string &type) : type(type) {}

	string type;
};

// NamedType
struct NamedType : Type {
	NamedType(const char *type, const char *name) 
		: Type(type), name(name) {}
	NamedType(const string &type, const string &name)
		: Type(type), name(name) {}

	string name;
};

// Function
class Function {
public:
	Function() : fReturnType("") {}

	void SetName(const string &name)
	{
		fName = name;
	}

	const string &GetName() const
	{
		return fName;
	}

	void AddParameter(const NamedType &type)
	{
		fParameters.push_back(type);
	}

	int CountParameters() const
	{
		return fParameters.size();
	}

	const NamedType &ParameterAt(int index) const
	{
		return fParameters[index];
	}

	void SetReturnType(const Type &type)
	{
		fReturnType = type;
	}

	const Type &GetReturnType() const
	{
		return fReturnType;
	}

protected:
	string				fName;
	vector<NamedType>	fParameters;
	Type				fReturnType;
};

// Syscall
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

// Tokenizer
class Tokenizer {
public:
	Tokenizer(istream &input)
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

	string GetNextToken(stack<string> &skippedTokens)
	{
		return GetNextToken(&skippedTokens);
	}

	string GetNextToken(stack<string> *skippedTokens)
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

	void ExpectToken(const string &expectedToken)
	{
		string token = GetCurrentToken();
		if (expectedToken != token) {
			throw ParseException(string("Unexpected token `") + token
				+ "'. Expected was `" + expectedToken + "'.");
		}
	}

	void ExpectNextToken(const string &expectedToken)
	{
		GetNextToken();
		ExpectToken(expectedToken);
	}

	bool CheckToken(const string &expectedToken)
	{
		string token = GetCurrentToken();
		return (expectedToken == token);
	}

	bool CheckNextToken(const string &expectedToken)
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

	void PutTokens(stack<string> &tokens)
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
	istream			&fInput;
	list<string>	fTokens;
	bool			fHasCurrent;
};


// Main
class Main {
public:

	int Run(int argc, char **argv)
	{
		// parse parameters
		if (argc >= 2
			&& (string(argv[1]) == "-h" || string(argv[1]) == "--help")) {
			print_usage(false);
			return 0;
		}
		if (argc != 3) {
			print_usage(true);
			return 1;
		}
		_ParseSyscalls(argv[1]);
		_WriteSyscallInfoFile(argv[2]);
		return 0;
	}


private:
	void _ParseSyscalls(const char *filename)
	{
		// open the input file
		ifstream file(filename, ifstream::in);
		if (!file.is_open())
			throw new IOException(string("Failed to open `") + filename + "'.");
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

	void _WriteSyscallInfoFile(const char *filename)
	{
		// open the syscall info file
		ofstream file(filename, ofstream::out | ofstream::trunc);
		if (!file.is_open())
			throw new IOException(string("Failed to open `") + filename + "'.");

		// write preamble
		file << "#include \"gensyscalls.h\"" << endl;
		file << "#include \"syscalls.h.pp\"" << endl;
		file << endl;

		file << "SyscallVector* create_syscall_vector() {" << endl;
		file << "\tSyscallVector* syscallVector = SyscallVector::Create();"
			<< endl;
		file << "\tSyscall* syscall;" << endl;

		// syscalls
		for (int i = 0; i < (int)fSyscalls.size(); i++) {
			const Syscall &syscall = fSyscalls[i];

			// syscall = syscallVector->CreateSyscall("syscallName",
			//	"syscallKernelName");
			file << "\tsyscall = syscallVector->CreateSyscall(\""
				<< syscall.GetName() << "\", \""
				<< syscall.GetKernelName() << "\");" << endl;
	
			const Type &returnType = syscall.GetReturnType();

			// syscall->SetReturnType<ReturnType>("returnType");
			file << "\tsyscall->SetReturnType<" << returnType.type
				<< ">(\"" << returnType.type << "\");" << endl;

			// parameters
			int paramCount = syscall.CountParameters();
			for (int k = 0; k < paramCount; k++) {
				const NamedType &param = syscall.ParameterAt(k);
				// syscall->AddParameter<ParameterType>("parameterTypeName",
				//	"parameterName");
				file << "\tsyscall->AddParameter<"
					<< param.type << ">(\""
					<< param.type << "\", \""
					<< param.name << "\");" << endl;
			}
			file << endl;
		}

		// postamble
		file << "\treturn syscallVector;" << endl;
		file << "}" << endl;
	}

	void _ParseSyscall(Tokenizer &tokenizer, Syscall &syscall)
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

	void _ParseParameter(Tokenizer &tokenizer, Syscall &syscall)
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

	void _ParseFunctionPointerParameter(Tokenizer &tokenizer, Syscall &syscall,
		vector<string> &type)
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

