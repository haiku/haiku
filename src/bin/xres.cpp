/*
 * Copyright 2005-2010, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include <List.h>
#include <Resources.h>
#include <StorageDefs.h>
#include <TypeConstants.h>


using namespace std;


static const char *kCommandName = "xres";
static const char *kDefaultResourceName = NULL;
static const char *kDefaultOutputFile = "xres.output.rsrc";
static const int kMaxSaneResourceSize = 100 * 1024 * 1024;	// 100 MB

static int kArgc;
static const char *const *kArgv;

// usage
const char *kUsage =
"Usage: %s ( -h | --help )\n"
"       %s -l <file> ...\n"
"       %s <command> ...\n"
"\n"
"The first form prints this help text and exits.\n"
"\n"
"The second form lists the resources of all given files.\n"
"\n"
"The third form manipulates the resources of one or more files according to\n"
"the given commands.\n"
"\n"
"Valid commands are:\n"
"  <input file>\n"
"         - Add the resources read from file <input file> to the current\n"
"           output file. The file can either be a resource file or an\n"
"           executable file.\n"
"  -a <type>:<id>[:<name>] ( <file> |  -s <data> )\n"
"         - Add a resource to the current output file. The added resource is\n"
"           of type <type> and has the ID <id>. If given the resource will\n"
"           have name <name>, otherwise it won't have a name. The resource\n"
"           data will either be the string <data> provided on the command\n"
"           line or the data read from file <file> (the whole contents).\n"
"  -d <type>[:<id>]\n"
"         - Excludes resources with type <type> and, if given, ID <id> from\n"
"           being written to the output file. This applies to all resources\n"
"           read from input files or directly specified via command \"-a\"\n"
"           following this command until the next \"-d\" command.\n"
"  -o <output file>\n"
"         - Changes the output file to <output file>. All resources specified\n"
"           by subsequent <input file> or \"-a\" commands will be written\n"
"           to this file until the next output file is specified via the\n"
"           \"-o\" command. Resources specified later overwrite earlier ones\n"
"           with the same type and ID. If <output file> doesn't exist yet, \n"
"           a resource file with the name will be created. If it exists and\n"
"           is an executable file, the resources will be added to it (if the\n"
"           file already has resources, they will be removed before). If it\n"
"           is a resource file or a file of unknown type, it will be\n"
"           overwritten with a resource file containing the specified\n"
"           resources. The initial output file is \"xres.output.rsrc\".\n"
"           Note that an output file will only be created or modified, if at\n"
"           least one <input file> or \"-a\" command is given for it.\n"
"  -x <type>[:<id>]\n"
"         - Only resources with type <type> and, if given, ID <id> will be\n"
"           written to the output file. This applies to all resources\n"
"           read from input files or directly specified via command \"-a\"\n"
"           following this command until the next \"-x\" command.\n"
"  --     - All following arguments, even if starting with a \"-\" character,\n"
"           are treated as input file names.\n"
"\n"
"Parameters:\n"
"  <type> - A type constant consisting of exactly four characters.\n"
"  <id>   - A positive or negative integer.\n"
;


// resource_type
static const char *
resource_type(type_code type)
{
	static char typeString[5];

	typeString[0] = type >> 24;
	typeString[1] = (type >> 16) & 0xff;
	typeString[2] = (type >> 8) & 0xff;
	typeString[3] = type & 0xff;
	typeString[4] = '\0';

	return typeString;
}


// ResourceID
struct ResourceID {
	type_code	type;
	int32		id;
	bool		wildcardID;

	ResourceID(type_code type = B_ANY_TYPE, int32 id = 0,
			bool wildcardID = true)
		:
		type(type),
		id(id),
		wildcardID(wildcardID)
	{
	}

	ResourceID(const ResourceID &other)
	{
		*this = other;
	}

	bool Matches(const ResourceID &other) const
	{
		return ((type == other.type || type == B_ANY_TYPE)
			&& (wildcardID || id == other.id));
	}

	ResourceID &operator=(const ResourceID &other)
	{
		type = other.type;
		id = other.id;
		wildcardID = other.wildcardID;
		return *this;
	}
};


// ResourceDataSource
struct ResourceDataSource {
	ResourceDataSource()
	{
	}

	virtual ~ResourceDataSource()
	{
	}

	virtual void GetData(const void *&data, size_t &size) = 0;

	virtual void Flush()
	{
	}
};


// MemoryResourceDataSource
struct MemoryResourceDataSource : ResourceDataSource {
	MemoryResourceDataSource(const void *data, size_t size, bool clone)
	{
		_Init(data, size, clone);
	}

	MemoryResourceDataSource(const char *data, bool clone)
	{
		_Init(data, strlen(data) + 1, clone);
	}

	virtual ~MemoryResourceDataSource()
	{
		if (fOwner)
			delete[] fData;
	}

	virtual void GetData(const void *&data, size_t &size)
	{
		data = fData;
		size = fSize;
	}

private:
	void _Init(const void *data, size_t size, bool clone)
	{
		if (clone) {
			fData = new uint8[size];
			memcpy(fData, data, size);
			fSize = size;
			fOwner = true;
		} else {
			fData = (uint8*)data;
			fSize = size;
			fOwner = false;
		}
	}

private:
	uint8	*fData;
	size_t	fSize;
	bool	fOwner;
};


// FileResourceDataSource
struct FileResourceDataSource : ResourceDataSource {
	FileResourceDataSource(const char *path)
		:
		fPath(path),
		fData(NULL),
		fSize(0)
	{
	}

	virtual ~FileResourceDataSource()
	{
		Flush();
	}

	virtual void GetData(const void *&_data, size_t &_size)
	{
		if (!fData) {
			// open the file for reading
			BFile file;
			status_t error = file.SetTo(fPath.c_str(), B_READ_ONLY);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to open file \"%s\": %s\n",
					fPath.c_str(), strerror(error));

				exit(1);
			}

			// get size
			off_t size;
			error = file.GetSize(&size);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to get size of file \"%s\": "
					"%s\n", fPath.c_str(), strerror(error));

				exit(1);
			}

			// check size
			if (size > kMaxSaneResourceSize) {
				fprintf(stderr, "Error: Resource data file \"%s\" is too big\n",
					fPath.c_str());

				exit(1);
			}

			// read the data
			fData = new uint8[size];
			fSize = size;
			ssize_t bytesRead = file.ReadAt(0, fData, fSize);
			if (bytesRead < 0) {
				fprintf(stderr, "Error: Failed to read data size from file "
					"\"%s\": %s\n", fPath.c_str(), strerror(bytesRead));

				exit(1);
			}
		}

		_data = fData;
		_size = fSize;
	}

	virtual void Flush()
	{
		if (fData) {
			delete[] fData;
			fData = NULL;
		}
	}

private:
	string	fPath;
	uint8	*fData;
	size_t	fSize;
};


// State
struct State {
	State()
	{
	}

	virtual ~State()
	{
	}

	virtual void SetOutput(const char *path)
	{
		(void)path;
	}

	virtual void ProcessInput(const char *path)
	{
		(void)path;
	}

	virtual void SetInclusionPattern(const ResourceID &pattern)
	{
		(void)pattern;
	}

	virtual void SetExclusionPattern(const ResourceID &pattern)
	{
		(void)pattern;
	}

	virtual void AddResource(const ResourceID &id, const char *name,
		ResourceDataSource *dataSource)
	{
		(void)id;
		(void)name;
		(void)dataSource;
	}
};


// ListState
struct ListState : State {
	ListState()
	{
	}

	virtual ~ListState()
	{
	}

	virtual void ProcessInput(const char *path)
	{
		// open the file for reading
		BFile file;
		status_t error = file.SetTo(path, B_READ_ONLY);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open file \"%s\": %s\n", path,
				strerror(error));
			exit(1);
		}

		// open the resources
		BResources resources;
		error = resources.SetTo(&file, false);
		if (error != B_OK) {
			if (error == B_ERROR) {
				fprintf(stderr, "Error: File \"%s\" is not a resource file.\n",
				path);
			} else {
				fprintf(stderr, "Error: Failed to read resources from file "
					"\"%s\": %s\n", path, strerror(error));
			}

			exit(1);
		}

		// print resources
		printf("\n%s resources:\n\n", path);
		printf(" type           ID        size  name\n");
		printf("------ ----------- -----------  --------------------\n");

		type_code type;
		int32 id;
		const char *name;
		size_t size;
		for (int32 i = 0;
				resources.GetResourceInfo(i, &type, &id, &name, &size); i++) {
			printf("'%s' %11" B_PRId32 " %11" B_PRIuSIZE "  %s\n",
				resource_type(type), id, size,
				name != NULL && name[0] != '\0' ? name : "(no name)");
		}
	}
};


// WriteFileState
struct WriteFileState : State {
	WriteFileState()
		:
		fOutputFilePath(kDefaultOutputFile),
		fResources(NULL),
		fInclusionPattern(NULL),
		fExclusionPattern(NULL)
	{
	}

	virtual ~WriteFileState()
	{
		_FlushOutput();
	}

	virtual void SetOutput(const char *path)
	{
		_FlushOutput();

		fOutputFilePath = path;
	}

	virtual void ProcessInput(const char *path)
	{
		// open the file for reading
		BFile file;
		status_t error = file.SetTo(path, B_READ_ONLY);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open input file \"%s\": %s\n",
				path, strerror(error));
			exit(1);
		}

		// open the resources
		BResources resources;
		error = resources.SetTo(&file, false);
		if (error != B_OK) {
			if (error == B_ERROR) {
				fprintf(stderr, "Error: Input file \"%s\" is not a resource "
				"file.\n", path);
			} else {
				fprintf(stderr, "Error: Failed to read resources from input "
					"file \"%s\": %s\n", path, strerror(error));
			}

			exit(1);
		}
		resources.PreloadResourceType();

		// add resources
		type_code type;
		int32 id;
		const char *name;
		size_t size;
		for (int32 i = 0;
			 resources.GetResourceInfo(i, &type, &id, &name, &size);
			 i++) {
			// load the resource
			const void *data = resources.LoadResource(type, id, &size);
			if (!data) {
				fprintf(stderr, "Error: Failed to read resources from input "
					"file \"%s\".\n", path);

				exit(1);
			}

			// add it
			MemoryResourceDataSource dataSource(data, size, false);
			AddResource(ResourceID(type, id), name, &dataSource);
		}
	}

	virtual void SetInclusionPattern(const ResourceID &pattern)
	{
		if (!fInclusionPattern)
			fInclusionPattern = new ResourceID;
		*fInclusionPattern = pattern;
	}

	virtual void SetExclusionPattern(const ResourceID &pattern)
	{
		if (!fExclusionPattern)
			fExclusionPattern = new ResourceID;
		*fExclusionPattern = pattern;
	}

	virtual void AddResource(const ResourceID &id, const char *name,
		ResourceDataSource *dataSource)
	{
		_PrepareOutput();

		// filter resource
		if ((fInclusionPattern && !fInclusionPattern->Matches(id))
			|| (fExclusionPattern && fExclusionPattern->Matches(id))) {
			// not included or explicitly excluded
			return;
		}

		// get resource data
		const void *data;
		size_t size;
		dataSource->GetData(data, size);

		// add the resource
		status_t error = fResources->AddResource(id.type, id.id, data, size,
			name);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to add resource type '%s', ID %"
				B_PRId32 " to output file \"%s\": %s\n", resource_type(id.type),
				id.id, fOutputFilePath.c_str(), strerror(error));
			exit(1);
		}
	}

private:
	void _FlushOutput()
	{
		if (fResources) {
			status_t error = fResources->Sync();
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to write resources to output "
					"file \"%s\": %s\n", fOutputFilePath.c_str(),
					strerror(error));

				exit(1);
			}

			delete fResources;
			fResources = NULL;
		}
	}

	void _PrepareOutput()
	{
		if (fResources)
			return;

		// open the file for writing
		BFile file;
		status_t error = file.SetTo(fOutputFilePath.c_str(),
			B_READ_WRITE | B_CREATE_FILE);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open output file \"%s\": %s\n",
				fOutputFilePath.c_str(), strerror(error));
			exit(1);
		}

		// open the resources
		fResources = new BResources;
		error = fResources->SetTo(&file, true);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to init resources for output "
				"file \"%s\": %s\n", fOutputFilePath.c_str(), strerror(error));

			exit(1);
		}
	}

private:
	string		fOutputFilePath;
	BResources	*fResources;
	ResourceID	*fInclusionPattern;
	ResourceID	*fExclusionPattern;
};


// Command
struct Command {
	Command()
	{
	}

	virtual ~Command()
	{
	}

	virtual void Do(State *state) = 0;
};


// SetOutputCommand
struct SetOutputCommand : Command {
	SetOutputCommand(const char *path)
		:
		Command(),
		fPath(path)
	{
	}

	virtual void Do(State *state)
	{
		state->SetOutput(fPath.c_str());
	}

private:
	string	fPath;
};


// ProcessInputCommand
struct ProcessInputCommand : Command {
	ProcessInputCommand(const char *path)
		:
		Command(),
		fPath(path)
	{
	}

	virtual void Do(State *state)
	{
		state->ProcessInput(fPath.c_str());
	}

private:
	string	fPath;
};


// SetResourcePatternCommand
struct SetResourcePatternCommand : Command {
	SetResourcePatternCommand(const ResourceID &pattern, bool inclusion)
		:
		Command(),
		fPattern(pattern),
		fInclusion(inclusion)
	{
	}

	virtual void Do(State *state)
	{
		if (fInclusion)
			state->SetInclusionPattern(fPattern);
		else
			state->SetExclusionPattern(fPattern);
	}

private:
	ResourceID	fPattern;
	bool		fInclusion;
};


// AddResourceCommand
struct AddResourceCommand : Command {
	AddResourceCommand(const ResourceID &id, const char *name,
			ResourceDataSource *dataSource)
		:
		Command(),
		fID(id),
		fHasName(name),
		fDataSource(dataSource)
	{
		if (fHasName)
			fName = name;
	}

	virtual ~AddResourceCommand()
	{
		delete fDataSource;
	}

	virtual void Do(State *state)
	{
		state->AddResource(fID, (fHasName ? fName.c_str() : NULL),
			fDataSource);
		fDataSource->Flush();
	}

private:
	ResourceID			fID;
	string				fName;
	bool				fHasName;
	ResourceDataSource	*fDataSource;
};


// print_usage
static void
print_usage(bool error)
{
	// get command name
	const char *commandName = NULL;
	if (kArgc > 0) {
		if (const char *lastSlash = strchr(kArgv[0], '/'))
			commandName = lastSlash + 1;
		else
			commandName = kArgv[0];
	}

	if (!commandName || commandName[0] == '\0')
		commandName = kCommandName;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, commandName, commandName,
		commandName);
}


// print_usage_and_exit
static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


// next_arg
static const char *
next_arg(int &argi, bool optional = false)
{
	if (argi >= kArgc) {
		if (!optional)
			print_usage_and_exit(true);
		return NULL;
	}

	return kArgv[argi++];
}


// parse_resource_id
static void
parse_resource_id(const char *toParse, ResourceID &resourceID,
	const char **name = NULL)
{
	int len = strlen(toParse);

	// type
	if (len < 4)
		print_usage_and_exit(true);

	resourceID.type = ((int32)toParse[0] << 24) | ((int32)toParse[1] << 16)
		| ((int32)toParse[2] << 8) | (int32)toParse[3];

	if (toParse[4] == '\0') {
		// if a name can be provided, the ID is mandatory
		if (name)
			print_usage_and_exit(true);

		resourceID.id = 0;
		resourceID.wildcardID = true;
		return;
	}

	if (toParse[4] != ':')
		print_usage_and_exit(true);

	toParse += 5;
	len -= 5;

	// ID
	bool negative = false;
	if (*toParse == '-') {
		negative = true;
		toParse++;
		len--;
	}

	if (*toParse < '0' || *toParse > '9')
		print_usage_and_exit(true);

	int id = 0;
	while (*toParse >= '0' && *toParse <= '9') {
		id = 10 * id + (*toParse - '0');
		toParse++;
		len--;
	}

	resourceID.wildcardID = false;
	resourceID.id = (negative ? -id : id);

	if (*toParse == '\0') {
		if (name)
			*name = kDefaultResourceName;
			return;
	}

	if (*toParse != ':')
		print_usage_and_exit(true);

	// the remainder is name
	*name = toParse + 1;
}


// main
int
main(int argc, const char *const *argv)
{
	kArgc = argc;
	kArgv = argv;

	if (argc < 2)
		print_usage_and_exit(true);

	BList commandList;

	// parse the arguments
	bool noMoreOptions = false;
	bool list = false;
	bool noList = false;
	bool hasInputFiles = false;
	for (int argi = 1; argi < argc; ) {
		const char *arg = argv[argi++];
		if (!noMoreOptions && arg[0] == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
				print_usage_and_exit(false);

			if (strlen(arg) != 2)
				print_usage_and_exit(true);

			switch (arg[1]) {
				case 'a':
				{
					noList = true;

					// get id
					const char *typeString = next_arg(argi);
					ResourceID resourceID;
					const char *name = NULL;
					parse_resource_id(typeString, resourceID, &name);

					// get data
					const char *file = next_arg(argi);
					ResourceDataSource *dataSource;
					if (strcmp(file, "-s") == 0) {
						const char *data = next_arg(argi);
						dataSource = new MemoryResourceDataSource(data, false);

					} else {
						dataSource = new FileResourceDataSource(file);
					}

					// add command
					Command *command = new AddResourceCommand(resourceID,
						name, dataSource);
					commandList.AddItem(command);

					break;
				}

				case 'd':
				{
					noList = true;

					// get pattern
					const char *typeString = next_arg(argi);
					ResourceID pattern;
					parse_resource_id(typeString, pattern);

					// add command
					Command *command = new SetResourcePatternCommand(pattern,
						false);
					commandList.AddItem(command);

					break;
				}

				case 'l':
				{
					list = true;
					break;
				}

				case 'o':
				{
					noList = true;

					// get file name
					const char *out = next_arg(argi);

					// add command
					Command *command = new SetOutputCommand(out);
					commandList.AddItem(command);

					break;
				}

				case 'x':
				{
					noList = true;

					// get pattern
					const char *typeString = next_arg(argi);
					ResourceID pattern;
					parse_resource_id(typeString, pattern);

					// add command
					Command *command = new SetResourcePatternCommand(pattern,
						true);
					commandList.AddItem(command);

					break;
				}

				case '-':
					noMoreOptions = true;
					break;

				default:
					print_usage_and_exit(true);
					break;
			}

		} else {
			// input file
			hasInputFiles = true;
			Command *command = new ProcessInputCommand(arg);
			commandList.AddItem(command);
		}
	}

	// don't allow "-l" together with other comands or without at least one
	// input file
	if ((list && noList) || (list && !hasInputFiles))
		print_usage_and_exit(true);

	// create a state
	State *state;
	if (list)
		state = new ListState();
	else
		state = new WriteFileState();

	// process commands
	for (int32 i = 0; Command *command = (Command*)commandList.ItemAt(i); i++)
		command->Do(state);

	// delete state (will flush resources)
	delete state;

	return 0;
}
