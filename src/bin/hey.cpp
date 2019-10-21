// hey
// a small scripting utility
// written by Attila Mezei (attila.mezei@mail.datanet.hu)
// contributions by Sander Stoks, Peter Folk, Chris Herborth, Marco Nelissen, Scott Lindsey and others
//
// public domain, use it at your own risk
//
// 1.2.8:	(Sander Stoks): Added command-line option -o which will output the "result" value
//		in the reply message to stdout, so you can use it in shell scripting more easily:
//		"hey Becasso get AspectRatio of Canvas 0"
//		outputs
//		Reply BMessage(B_REPLY):
//		   "result" (B_DOUBLE_TYPE) : 0.600
//		but "hey -o Becasso get AspectRatio of Canvas 0"
//		outputs 0.600000 directly.
//
// 1.2.7:	by Sander Stoks: Made a fork since I don't think Attila still supports "hey", and
//		because the latest version on BeBits seems to be 1.2.4.
//		Changes w.r.t. 1.2.6: When an application returns an error on a message, hey now
//		keeps iterating over applications with the same signature.  This is useful because,
//		for instance, Terminal starts as a new process for each instance, so it previously
//		wouldn't work to move a specific Terminal window using hey.  You can now say
//		"hey Terminal set Frame of Window foo to BRect[...]".
//
// 1.2.6:	syntax extended by Sander Stoks <sander@stoks.nl to allow:
//		"hey Application let Specifier do ..."
//		allowing you to send messages directly to other handlers than the app itself.
//		In cooperation with the new Application defined commands (note that some
//		Be classes, e.g. BWindow, publish commands as well) this allows, for example:
//		"hey NetPositive let Window 0 do MoveBy BPoint[10,10]"
//		Note that this is partly redundant, since
//		"hey NetPositive let Window 0 do get Title"
//		duplicates "hey NetPositive get Title of Window 0"
//		But with the old system,
//		"hey NetPositive MoveBy of Window 0 with data=BPoint[10,10]"
//		couldn't work ("MoveBy" is not defined by the Application itself).
//
// 1.2.5:   value_info is supported in BPropertyInfo. This means when you send GETSUITES (B_GET_SUPPORTED_SUITES)
//	the value info is printed after the property infos, like this:
//		   "messages" (B_PROPERTY_INFO_TYPE) :
//		        property   commands                            specifiers
//		--------------------------------------------------------------------------------
//		          Suites   B_GET_PROPERTY                      DIRECT
//		       Messenger   B_GET_PROPERTY                      DIRECT
//		    InternalName   B_GET_PROPERTY                      DIRECT
//
//		            name   value                               kind
//		--------------------------------------------------------------------------------
//		          Backup   0x6261636B ('back')                 COMMAND
//		                   Usage: This command backs up your hard drive.
//		           Abort   0x61626F72 ('abor')                 COMMAND
//		                   Usage: Stops the current operation...
//		       Type Code   0x74797065 ('type')                 TYPE CODE
//		                   Usage: Type code info...
//
//	You can also use the application defined commands (as the verb) with hey:
//		hey MyBackupApp Backup "Maui"
//
// 1.2.4:   the syntax is extended by Peter Folk <pfolk@uni.uiuc.edu> to contain:
//      do the x of y -3 of z '"1"'
//      I.e. "do" => B_EXECUTE_PROPERTY, optional "the" makes direct specifiers
//      more like english, bare reverse-index-specifiers are now handled, and
//      named specifiers can contain only digits by quoting it (but make sure the
//      shell passed the quotes through).
//
//      Hey(target,const char*,reply) was previously limited to 100 tokens.  It
//      now uses a vector<> so it's only limited by available memory.
//
//      Also, the archive name is now Y2K compliant =)
//
// 1.2.3:	new option: -s for silent processing (no reply or errors printed) AM
//
// 1.2.2:	Fixes by Marco Nelissen (marcone@xs4all.nl)
//      - fixed parsing of negative numbers
//		- fixed "with" syntax, which was broken (after a create, "with" would be taken as a specifier)
//
// 1.2.1:	compiled for x86, R4 with minor modifications at BPropertyInfo
//
// 1.2.0:	the syntax is extended by Sander Stoks (sander@adamation.com) to contain
//		with name=<value> [and name=<value> [...]]
//		at the end of the command which will add additional data to the scripting message. E.g:
//		hey Becasso create Canvas with name=MyCanvas and size=BRect(100,100,300,300)
//		Also a small interpreter is included.
//
//		Detailed printout of B_PROPERTY_INFO in BMessages. Better than BPropertyInfo::PrintToStream().
//		Also prints usage info for a property if defined.
//
// 1.1.1:	minor change from chrish@qnx.com to return -1 if an error is
//		sent back in the reply message; also added B_COUNT_PROPERTIES support
//
//		The range specifier sent to the target was 1 greater than it should've been. Fixed.
//
//		'hey' made the assumption that the first thread in a team will be the
//		application thread (and therefore have the application's name).
//		This was not always the case. Fix from Scott Lindsey <wombat@gobe.com>.
//
//v1.1.0:	Flattened BPropertyInfo is printed if found in the reply of B_GET_SUPPORTED_SUITES
//		1,2,3 and 4 character message constant is supported (e.g. '1', '12', '123', '1234')
//		Alpha is sent with rgb_color
//
//v1.0.0	First public release


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <AppKit.h>
#include <Path.h>
#include <SupportDefs.h>

int32 HeyInterpreterThreadHook(void* arg);

status_t Hey(BMessenger* target, const char* arg, BMessage* reply);
bool isSpace(char c);
status_t Hey(BMessenger* target, char* argv[], int32* argx, int32 argc, BMessage* reply);
status_t add_specifier(BMessage *to_message, char *argv[], int32 *argx, int32 argc);
status_t add_data(BMessage *to_message, char *argv[], int32 *argx);
status_t add_with(BMessage *to_message, char *argv[], int32 *argx, int32 argc);
void add_message_contents(BList *textlist, BMessage *msg, int32 level);
char *get_datatype_string(int32 type);
char *format_data(int32 type, char *ptr, long size);
void print_message(BMessage *message);
char *id_to_string(long ID, char *here);
bool is_valid_char(uint8 c);

const char VERSION[] = "v1.2.8";

#define MAX_INPUT_SIZE 1024
	// Maximum amount of input data that "hey" can process at a time

#define DEBUG_HEY 0		// 1: prints the script message to be sent to the target application, 0: prints only the reply


// test, these should be zero for normal operation
#define TEST_VALUEINFO 0


// flag for silent mode
bool silent;
// flag for stdout mode
bool output;

status_t
parse(BMessenger& the_application, int argc, char *argv[], int32 argapp)
{
	if (!the_application.IsValid()) {
		if (!silent)
			fprintf(stderr, "Cannot find the application (%s)\n", argv[argapp]);
		return B_ERROR;
	}

	if (argc < 3) {
		if (!silent)
			fprintf(stderr, "Cannot find the verb!\n");
		return B_ERROR;
	}


	BMessage the_reply;
	int32 argx = argapp+1;
	status_t err = Hey(&the_application, argv, &argx, argc, &the_reply);

	if (err != B_OK) {
		if (!silent) {
			fprintf(stderr, "Error when sending message to %s!\n",
				argv[argapp]);
		}
		return B_ERROR;
	} else {
		if (the_reply.what == (uint32)B_MESSAGE_NOT_UNDERSTOOD
			|| the_reply.what == (uint32)B_ERROR) {	// I do it myself
			if (the_reply.HasString("message")) {
				if (!silent) {
					printf("%s (error 0x%8" B_PRIx32 ")\n",
						the_reply.FindString("message"),
						the_reply.FindInt32("error"));
				}
			} else {
				if (!silent) {
					printf("error 0x%8" B_PRIx32 "\n",
						the_reply.FindInt32("error"));
				}
			}
			return 1;
		} else {
			if (!silent) {
				if (output) {
					type_code tc;
					if (the_reply.GetInfo("result", &tc) == B_OK) {
						if (tc == B_INT8_TYPE) {
							int8 v;
							the_reply.FindInt8("result", &v);
							printf("%d\n", v);
						} else if (tc == B_INT16_TYPE) {
							int16 v;
							the_reply.FindInt16("result", &v);
							printf("%d\n", v);
						} else if (tc == B_INT32_TYPE) {
							int32 v;
							the_reply.FindInt32("result", &v);
							printf("%" B_PRId32 "\n", v);
						} else if (tc == B_UINT8_TYPE) {
							uint8 v;
							the_reply.FindInt8("result", (int8*)&v);
							printf("%u\n", v);
						} else if (tc == B_UINT16_TYPE) {
							uint16 v;
							the_reply.FindInt16("result", (int16*)&v);
							printf("%u\n", v);
						} else if (tc == B_UINT32_TYPE) {
							uint32 v;
							the_reply.FindInt32("result", (int32*)&v);
							printf("%" B_PRIu32 "\n", v);
						} else if (tc == B_STRING_TYPE) {
							const char* v;
							the_reply.FindString("result", &v);
							printf("%s\n", v);
						} else if (tc == B_FLOAT_TYPE) {
							float v;
							the_reply.FindFloat("result", &v);
							printf("%f\n", v);
						} else if (tc == B_DOUBLE_TYPE) {
							double v;
							the_reply.FindDouble("result", &v);
							printf("%f\n", v);
						} else if (tc == B_BOOL_TYPE) {
							bool v;
							the_reply.FindBool("result", &v);
							printf("%s\n", v ? "true" : "false");
						} else
							printf("Unsupported type\n");
					}
				} else {
					printf("Reply ");
					print_message(&the_reply);
					printf("\n");
				}
			}
		}
	}
	return B_OK;
}


void
usage(int exitCode)
{
	fprintf(exitCode == EXIT_SUCCESS ? stdout : stderr,
		"hey %s, written by Attila Mezei (attila.mezei@mail.datanet.hu)\n"
		"usage: hey [-s][-o] <app|signature|teamid> [let <specifier> do] <verb> <specifier_1> <of\n"
		"           <specifier_n>>* [to <value>] [with name=<value> [and name=<value>]*]\n"
		"where  <verb> : DO|GET|SET|COUNT|CREATE|DELETE|GETSUITES|QUIT|SAVE|LOAD|'what'\n"
		"  <specifier> : [the] <property_name> [ <index> | name | \"name\" | '\"name\"' ]\n"
		"      <index> : int | -int | '['int']' | '['-int']' | '['startint to end']'\n"
		"      <value> : \"string\" | <integer> | <float> | bool(value) | int8(value)\n"
		"                | int16(value) | int32(value) | float(value) | double(value)\n"
		"                | BPoint(x,y) | BRect(l,t,r,b) | rgb_color(r,g,b,a) | file(path)\n"
		"options: -s: silent\n"
		"         -o: output result to stdout for easy parsing\n\n", VERSION);
	exit(exitCode);
}


int
main(int argc, char *argv[])
{
	BApplication app("application/x-amezei-hey");

	if (argc < 2)
		usage(1);

	int32 argapp = 1;
	silent = false;
	output = false;

	// Updated option mechanism
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "-S") == 0) {
			silent = true;
			argapp++;
		} else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "-O") == 0) {
			output = true;
			argapp++;
		} else  if (strcmp(argv[1], "-h") == 0
			|| strcmp(argv[1], "--help") == 0)
			usage(0);
	}

	// find the application
	BMessenger the_application;
	BList team_list;
	team_id teamid;
	app_info appinfo;

	teamid = atoi(argv[argapp]);
	if (teamid > 0) {
		if (be_roster->GetRunningAppInfo(teamid, &appinfo) != B_OK)
			return 1;
		the_application=BMessenger(NULL, teamid);
		if (!parse(the_application, argc, argv, argapp))
			return 0;
		return 1;
	}

	be_roster->GetAppList(&team_list);

	for (int32 i = 0; i < team_list.CountItems(); i++) {
		teamid = (team_id)(addr_t)team_list.ItemAt(i);
		be_roster->GetRunningAppInfo(teamid, &appinfo);
		if (strcmp(appinfo.signature, argv[argapp]) == 0) {
			the_application=BMessenger(appinfo.signature);
			if (!parse(the_application, argc, argv, argapp))
				return 0;
		} else {
			if (strcmp(appinfo.ref.name, argv[argapp]) == 0) {
				the_application = BMessenger(0, teamid);
				if (!parse(the_application, argc, argv, argapp))
					return 0;
			}
		}
	}

	return 1;
}


int32
HeyInterpreterThreadHook(void* arg)
{
	if (!arg)
		return 1;

	BMessage environment(*(BMessage*) arg);
	const char* prompt = "Hey";
	if (environment.HasString("prompt"))
		environment.FindString("prompt", &prompt);
	printf("%s> ", prompt);

	BMessenger target;
	if (environment.HasMessenger("Target"))
		environment.FindMessenger("Target", &target);

	char command[MAX_INPUT_SIZE];
	status_t err;
	BMessage reply;
	while (fgets(command, sizeof(command), stdin)) {
		reply.MakeEmpty();
		err = Hey(&target, command, &reply);
		if (!err) {
			print_message(&reply);
		} else {
			printf("Error!\n");
		}
		printf("%s> ", prompt);
	}

	return 0;
}

status_t
Hey(BMessenger* target, const char* arg, BMessage* reply)
{
	BList argv;
	char* tokens = new char[strlen(arg) * 2];
		// number of tokens is limited only by memory
	char* currentToken = tokens;
	int32 tokenNdex = 0;
	int32 argNdex = 0;
	bool inquotes = false;

	while (arg[argNdex] != 0) { // for each character in arg
		if (arg[argNdex] == '\"')
			inquotes = !inquotes;
		if (!inquotes && isSpace(arg[argNdex])) { // if the character is white space
			if (tokenNdex != 0) { //  close off currentToken token
				currentToken[tokenNdex] = 0;
				argv.AddItem(currentToken);
				currentToken += tokenNdex + 1;
				tokenNdex = 0;
				argNdex++;
			} else { // just skip the whitespace
				argNdex++;
			}
		} else { // copy char into current token
			currentToken[tokenNdex] = arg[argNdex];
			tokenNdex++;
			argNdex++;
		}
	}

	if (tokenNdex!=0) { //  close off currentToken token
		currentToken[tokenNdex] = 0;
		argv.AddItem(currentToken);
	}
	argv.AddItem(NULL);

	int32 argx = 0;
	status_t ret = Hey(target, (char **)argv.Items(), &argx, argv.CountItems() - 1, reply);
	  // This used to be "return Hey(...);"---so tokens wasn't delete'd.
	delete[] tokens;
	return ret;
}


bool
isSpace(char c)
{
	switch (c) {
		case ' ':
		case '\t':
			return true;

		default:
			return false;
	}
}


status_t
Hey(BMessenger* target, char* argv[], int32* argx, int32 argc, BMessage* reply)
{
	bool direct_what = false;
	BMessage the_message;
	if (strcasecmp(argv[*argx], "let") == 0) {
		BMessage get_target (B_GET_PROPERTY);
		get_target.AddSpecifier ("Messenger");
			// parse the specifiers
		(*argx)++;
		status_t result = B_OK;
		while ((result = add_specifier(&get_target, argv, argx, argc)) == B_OK)
			;

		if (result != B_ERROR) {
			if (!silent)
				fprintf(stderr, "Bad specifier syntax!\n");
			return result;
		}
		BMessage msgr;
		if (target && target->IsValid()) {
			result = target->SendMessage(&get_target, &msgr);
			if (result != B_OK)
				return result;
			result = msgr.FindMessenger ("result", target);
			if (result != B_OK) {
				if (!silent)
					fprintf(stderr, "Couldn't retrieve the BMessenger!\n");
				return result;
			}
		}
		if (!argv[*argx]) {
			if (!silent)
				fprintf(stderr, "Syntax error - forgot \"do\"?\n");
			return B_ERROR;
		}
	}
	if (strcasecmp(argv[*argx], "do") == 0)
		the_message.what = B_EXECUTE_PROPERTY;
	else if (strcasecmp(argv[*argx], "get") == 0)
		the_message.what = B_GET_PROPERTY;
	else if (strcasecmp(argv[*argx], "set") == 0)
		the_message.what = B_SET_PROPERTY;
	else if (strcasecmp(argv[*argx], "create") == 0)
		the_message.what = B_CREATE_PROPERTY;
	else if (strcasecmp(argv[*argx], "delete") == 0)
		the_message.what = B_DELETE_PROPERTY;
	else if (strcasecmp(argv[*argx], "quit") == 0)
		the_message.what = B_QUIT_REQUESTED;
	else if (strcasecmp(argv[*argx], "save") == 0)
		the_message.what = B_SAVE_REQUESTED;
	else if (strcasecmp(argv[*argx], "load") == 0)
		the_message.what = B_REFS_RECEIVED;
	else if (strcasecmp(argv[*argx], "count") == 0)
		the_message.what = B_COUNT_PROPERTIES;
	else if (strcasecmp(argv[*argx], "getsuites") == 0)
		the_message.what = B_GET_SUPPORTED_SUITES;
	else {
		switch(strlen(argv[*argx])) {
				// can be a message constant if 1,2,3 or 4 chars
			case 1:
				the_message.what = (int32)argv[*argx][0];
				break;
			case 2:
				the_message.what = (((int32)argv[*argx][0]) << 8)
					| (((int32)argv[*argx][1]));
				break;
			case 3:
				the_message.what = (((int32)argv[*argx][0]) << 16)
					| (((int32)argv[*argx][1]) << 8)
					| (((int32)argv[*argx][2]));
				break;
			case 4:
				the_message.what = (((int32)argv[*argx][0]) << 24)
					| (((int32)argv[*argx][1]) << 16)
					| (((int32)argv[*argx][2]) << 8)
					| (((int32)argv[*argx][3]));
				break;
			default:
				// maybe this is a user defined command, ask for the supported suites
				bool found = false;
				if (target && target->IsValid()) {
					BMessage reply;
					if (target->SendMessage(B_GET_SUPPORTED_SUITES, &reply)
						== B_OK) {
						// if all goes well, reply contains all kinds of
						// property infos
						int32 j = 0;
						void *voidptr;
						ssize_t sizefound;
						BPropertyInfo propinfo;
						const value_info *vinfo;
						int32 vinfo_index, vinfo_count;

//						const char *str;
//						while (rply.FindString("suites", j++, &str) == B_OK)
//							printf ("Suite %ld: %s\n", j, str);
//
//						j = 0;
						while (reply.FindData("messages", B_PROPERTY_INFO_TYPE,
								j++, (const void **)&voidptr, &sizefound)
							== B_OK && !found) {
							if (propinfo.Unflatten(B_PROPERTY_INFO_TYPE,
									(const void *)voidptr, sizefound) == B_OK) {
								vinfo = propinfo.Values();
								vinfo_index = 0;
								vinfo_count = propinfo.CountValues();
#if TEST_VALUEINFO>0
								value_info vinfo[10] = {
									{"Backup", 'back', B_COMMAND_KIND,
										"This command backs up your hard"
										" drive."},
									{"Abort", 'abor', B_COMMAND_KIND,
										"Stops the current operation..."},
									{"Type Code", 'type', B_TYPE_CODE_KIND,
										"Type code info..."}
								};
								vinfo_count = 3;
#endif

								while (vinfo_index < vinfo_count) {
									if (strcmp(vinfo[vinfo_index].name,
											argv[*argx]) == 0) {
										found = true;
										the_message.what =
											vinfo[vinfo_index].value;
#if TEST_VALUEINFO>0
										printf("FOUND COMMAND \"%s\" = %lX\n",
											vinfo[vinfo_index].name,
											the_message.what);
#endif
										break;
									}
									vinfo_index++;
								}
							}
						}
					}
				}

				if (!found) {
					if (!silent)
						fprintf(stderr, "Bad verb (\"%s\")\n", argv[*argx]);
					return -1;
				}
		}
		direct_what = true;
	}

	status_t result = B_OK;
	(*argx)++;

	// One exception: Single data item at end of line.
	if (direct_what && *argx == argc - 1 && argv[*argx] != NULL)
		add_data(&the_message, argv, argx);
	else {
		// parse the specifiers
		if (the_message.what != B_REFS_RECEIVED) {
				// LOAD has no specifier
			while ((result = add_specifier(&the_message, argv, argx, argc))
				== B_OK)
				;

			if (result != B_ERROR) {
				if (!silent)
					fprintf(stderr, "Bad specifier syntax!\n");
				return result;
			}
		}
	}

	// if verb is SET or LOAD, there should be a to <value>
	if ((the_message.what == B_SET_PROPERTY || the_message.what == B_REFS_RECEIVED) && argv[*argx] != NULL) {
		if (strcasecmp(argv[*argx], "to") == 0)
			(*argx)++;
		result = add_data(&the_message, argv, argx);
		if (result != B_OK) {
			if (result == B_FILE_NOT_FOUND) {
				if (!silent)
					fprintf(stderr, "File not found!\n");
			} else if (!silent)
				fprintf(stderr, "Invalid 'to...' value format!\n");
			return result;
		}
	}

	add_with(&the_message, argv, argx, argc);

#if DEBUG_HEY>0
	fprintf(stderr, "Send ");
	print_message(&the_message);
	fprintf(stderr, "\n");
#endif

	if (target && target->IsValid()) {
		if (reply)
			result = target->SendMessage(&the_message, reply);
		else
			result = target->SendMessage(&the_message);
	}
	return result;
}

// There can be a with <name>=<type>() [and <name>=<type> ...]
// I treat "and" just the same as "with", it's just to make the script syntax more English-like.
status_t
add_with(BMessage *to_message, char *argv[], int32 *argx, int32 argc)
{
	status_t result = B_OK;
	if (*argx < argc - 1 && argv[++(*argx)] != NULL) {
		// printf ("argv[%ld] = %s\n", *argx, argv[*argx]);
		if (strcasecmp(argv[*argx], "with") == 0) {
			// printf ("\"with\" detected!\n");
			(*argx)++;
			bool done = false;
			do {
				result = add_data(to_message, argv, argx);
				if (result != B_OK) {
					if (result == B_FILE_NOT_FOUND) {
						if (!silent)
							fprintf(stderr, "File not found!\n");
					} else {
						if (!silent)
							fprintf(stderr, "Invalid 'with...' value format!\n");
					}
					return result;
				}
				(*argx)++;
				// printf ("argc = %d, argv[%d] = %s\n", argc, *argx, argv[*argx]);
				if (*argx < argc - 1 && strcasecmp(argv[*argx], "and") == 0)
					(*argx)++;
				else
					done = true;
			} while (!done);
		}
	}
	return result;
}

// returns B_OK if successful
//         B_ERROR if no more specifiers
//         B_BAD_SCRIPT_SYNTAX if syntax error
status_t
add_specifier(BMessage *to_message, char *argv[], int32 *argx, int32 argc)
{
	char *property = argv[*argx];

	if (property == NULL)
		return B_ERROR;		// no more specifiers

	(*argx)++;

	if (strcasecmp(property, "do") == 0) {
			// Part of the "hey App let Specifier do Verb".
		return B_ERROR;	// no more specifiers
	}

	if (strcasecmp(property, "to") == 0) {
		return B_ERROR;
			// no more specifiers
	}

	if (strcasecmp(property, "with") == 0) {
		*argx -= 2;
		add_with(to_message, argv, argx, argc);
		return B_ERROR;
			// no more specifiers
	}

	if (strcasecmp(property, "of") == 0) {
			// skip "of", read real property
		property = argv[*argx];
		if (property == NULL)
			return B_BAD_SCRIPT_SYNTAX;
		(*argx)++;
	}

	if (strcasecmp(property, "the") == 0) {
			// skip "the", read real property
		property = argv[*argx];
		if (property == NULL)
			return B_BAD_SCRIPT_SYNTAX;
		(*argx)++;
	}

	// decide the specifier

	char *specifier = NULL;
	if (to_message->what == B_CREATE_PROPERTY) {
			// create is always direct. without this, a "with" would be
			// taken as a specifier
		(*argx)--;
	} else
		specifier = argv[*argx];
	if (specifier == NULL) {
			// direct specifier
		to_message->AddSpecifier(property);
		return B_ERROR;
			// no more specifiers
	}

	(*argx)++;

	if (strcasecmp(specifier, "of") == 0) {
			// direct specifier
		to_message->AddSpecifier(property);
		return B_OK;
	}

	if (strcasecmp(specifier, "to") == 0) {
			// direct specifier
		to_message->AddSpecifier(property);
		return B_ERROR;
			// no more specifiers
	}


	if (specifier[0] == '[') {
			// index, reverse index or range
		char *end;
		int32 ix1, ix2;
		if (specifier[1] == '-') {
				// reverse index
			ix1 = strtoul(specifier + 2, &end, 10);
			BMessage revspec(B_REVERSE_INDEX_SPECIFIER);
			revspec.AddString("property", property);
			revspec.AddInt32("index", ix1);
			to_message->AddSpecifier(&revspec);
		} else {
				// index or range
			ix1 = strtoul(specifier + 1, &end, 10);
			if (end[0] == ']') {
					// it was an index
				to_message->AddSpecifier(property, ix1);
				return B_OK;
			} else {
				specifier = argv[*argx];
				if (specifier == NULL) {
						// I was wrong, it was just an index
					to_message->AddSpecifier(property, ix1);
					return B_OK;
				}
				(*argx)++;
				if (strcasecmp(specifier, "to") == 0) {
					specifier = argv[*argx];
					if (specifier == NULL)
						return B_BAD_SCRIPT_SYNTAX;
					(*argx)++;
					ix2 = strtoul(specifier, &end, 10);
					to_message->AddSpecifier(property, ix1, ix2 - ix1 > 0
							? ix2 - ix1 : 1);
					return B_OK;
				} else
					return B_BAD_SCRIPT_SYNTAX;
			}
		}
	} else {
		// name specifier
		// if it contains only digits, it will be an index...
		bool index_spec = true;
		bool reverse = specifier[0] == '-';
		// accept bare reverse-index-specs
		size_t speclen = strlen(specifier);
		for (int32 i = (reverse ? 1 : 0); i < (int32)speclen; ++i) {
			if (specifier[i] < '0' || specifier[i] > '9') {
				index_spec = false;
				break;
			}
		}

		if (index_spec) {
			if (reverse) {
				// Copied from above
				BMessage revspec(B_REVERSE_INDEX_SPECIFIER);
				revspec.AddString("property", property);
				revspec.AddInt32("index", atol(specifier + 1));
				to_message->AddSpecifier(&revspec);
			} else
				to_message->AddSpecifier(property, atol(specifier));
		} else {
			// Allow any name by counting an initial " as a literal-string
			// indicator
			if (specifier[0] == '\"') {
				if (specifier[speclen - 1] == '\"')
					specifier[speclen - 1] = '\0';
				++specifier;
				--speclen;
			}
			to_message->AddSpecifier(property, specifier);
		}
	}

	return B_OK;
}


status_t
add_data(BMessage *to_message, char *argv[], int32 *argx)
{
	char *valuestring = argv[*argx];

	if (valuestring == NULL)
		return B_ERROR;

	// try to interpret it as an integer or float
	bool contains_only_digits = true;
	bool is_floating_point = false;
	for (int32 i = 0; i < (int32)strlen(valuestring); i++) {
		if (i != 0 || valuestring[i] != '-') {
			if (valuestring[i] < '0' || valuestring[i] > '9') {
				if (valuestring[i] == '.') {
					is_floating_point = true;
				} else {
					contains_only_digits = false;
					break;
				}
			}
		}
	}
	//printf("%d %d\n",	contains_only_digits,is_floating_point);
	if (contains_only_digits) {
		if (is_floating_point) {
			to_message->AddFloat("data", atof(valuestring));
			return B_OK;
		} else {
			to_message->AddInt32("data", atol(valuestring));
			return B_OK;
		}
	}

	// if true or false, it is bool
	if (strcasecmp(valuestring, "true") == 0) {
		to_message->AddBool("data", true);
		return B_OK;
	} else if (strcasecmp(valuestring, "false") == 0) {
		to_message->AddBool("data", false);
		return B_OK;
	}

	// Add support for "<name>=<type>()" here:
	// The type is then added under the name "name".

	#define MAX_NAME_LENGTH 128
	char curname[MAX_NAME_LENGTH];
	strcpy (curname, "data");	// This is the default.

	char *s = valuestring;
	while (*++s && *s != '=')
		// Look for a '=' character...
		;
	if (*s == '=') {	// We found a <name>=
		*s = 0;
		strcpy (curname, valuestring);	// Use the new <name>
		valuestring = s + 1;			// Reposition the valuestring ptr.
	}

	// must begin with a type( value )
	if (strncasecmp(valuestring, "int8", strlen("int8")) == 0)
		to_message->AddInt8(curname, atol(valuestring + strlen("int8(")));
	else if (strncasecmp(valuestring, "int16", strlen("int16")) == 0)
		to_message->AddInt16(curname, atol(valuestring + strlen("int16(")));
	else if (strncasecmp(valuestring, "int32", strlen("int32")) == 0)
		to_message->AddInt32(curname, atol(valuestring + strlen("int32(")));
	else if (strncasecmp(valuestring, "int64", strlen("int64")) == 0)
		to_message->AddInt64(curname, atol(valuestring + strlen("int64(")));
	else if (strncasecmp(valuestring, "bool", strlen("bool")) == 0) {
		if (strncasecmp(valuestring + strlen("bool("), "true", 4) == 0)
			to_message->AddBool(curname, true);
		else if (strncasecmp(valuestring + strlen("bool("), "false", 5) == 0)
			to_message->AddBool(curname, false);
		else
			to_message->AddBool(curname, atol(valuestring + strlen("bool(")) == 0 ? false : true);
	} else if (strncasecmp(valuestring, "float", strlen("float")) == 0)
		to_message->AddFloat(curname, atof(valuestring + strlen("float(")));
	else if (strncasecmp(valuestring, "double", strlen("double")) == 0)
		to_message->AddDouble(curname, atof(valuestring + strlen("double(")));
	else if (strncasecmp(valuestring, "BPoint", strlen("BPoint")) == 0) {
		float x, y;
		x = atof(valuestring + strlen("BPoint("));
		if (strchr(valuestring, ','))
			y = atof(strchr(valuestring, ',') + 1);
		else if (strchr(valuestring, ' '))
			y = atof(strchr(valuestring, ' ') + 1);
		else	// bad syntax
			y = 0.0f;
		to_message->AddPoint(curname, BPoint(x,y));
	} else if (strncasecmp(valuestring, "BRect", strlen("BRect")) == 0) {
		float l = 0.0f, t = 0.0f, r = 0.0f, b = 0.0f;
		char *ptr;
		l = atof(valuestring + strlen("BRect("));
		ptr = strchr(valuestring, ',');
		if (ptr) {
			t = atof(ptr + 1);
			ptr = strchr(ptr + 1, ',');
			if (ptr) {
				r = atof(ptr + 1);
				ptr = strchr(ptr + 1, ',');
				if (ptr)
					b = atof(ptr + 1);
			}
		}

		to_message->AddRect(curname, BRect(l,t,r,b));
	} else if (strncasecmp(valuestring, "rgb_color", strlen("rgb_color")) == 0) {
		rgb_color clr;
		char *ptr;
		clr.red = atol(valuestring + strlen("rgb_color("));
		ptr = strchr(valuestring, ',');
		if (ptr) {
			clr.green = atol(ptr + 1);
			ptr = strchr(ptr + 1, ',');
			if (ptr) {
				clr.blue = atol(ptr + 1);
				ptr = strchr(ptr + 1, ',');
				if (ptr)
					clr.alpha = atol(ptr + 1);
			}
		}

		to_message->AddData(curname, B_RGB_COLOR_TYPE, &clr, sizeof(rgb_color));
	} else if (strncasecmp(valuestring, "file", strlen("file")) == 0) {
		entry_ref file_ref;

		// remove the last ] or )
		if (valuestring[strlen(valuestring) - 1] == ')'
			|| valuestring[strlen(valuestring) - 1] == ']')
			valuestring[strlen(valuestring) - 1] = 0;

		if (get_ref_for_path(valuestring + 5, &file_ref) != B_OK)
			return B_FILE_NOT_FOUND;

		// check if the ref is valid
		BEntry entry;
		if (entry.SetTo(&file_ref) != B_OK)
			return B_FILE_NOT_FOUND;
		//if(!entry.Exists())  return B_FILE_NOT_FOUND;

		// add both ways, refsreceived needs it as "refs" while scripting needs "data"
		to_message->AddRef("refs", &file_ref);
		to_message->AddRef(curname, &file_ref);
	} else {
		// it is string
		// does it begin with a quote?
		if (valuestring[0] == '\"') {
			if (valuestring[strlen(valuestring) - 1] == '\"')
				valuestring[strlen(valuestring) - 1] = 0;
			to_message->AddString(curname, valuestring + 1);
		} else
			to_message->AddString(curname, valuestring);
	}

	return B_OK;
}


void
print_message(BMessage *message)
{
	BList textlist;
	add_message_contents(&textlist, message, 0);

	char *whatString = get_datatype_string(message->what);
	printf("BMessage(%s):\n", whatString);
	delete[] whatString;
	for (int32 i = 0; i < textlist.CountItems(); i++) {
		printf("   %s\n", (char*)textlist.ItemAt(i));
		free(textlist.ItemAt(i));
	}
}


void
add_message_contents(BList *textlist, BMessage *msg, int32 level)
{
	int32 count;
	int32 i, j;
	type_code typefound;
	ssize_t sizefound;
#ifdef HAIKU_TARGET_PLATFORM_DANO
	const char *namefound;
#else
	char *namefound;
#endif
	void *voidptr;
	BMessage a_message;
	char *textline, *datatype, *content;

	// go though all message data
	count = msg->CountNames(B_ANY_TYPE);
	for (i=0; i < count; i++) {
		msg->GetInfo(B_ANY_TYPE, i, &namefound, &typefound);
		j = 0;

		while (msg->FindData(namefound, typefound, j++, (const void **)&voidptr,
				&sizefound) == B_OK) {
			datatype = get_datatype_string(typefound);
			content = format_data(typefound, (char*)voidptr, sizefound);
			textline = (char*)malloc(20 + level * 4 + strlen(namefound)
					+ strlen(datatype) + strlen(content));
			memset(textline, 32, 20 + level * 4);
			sprintf(textline + level * 4, "\"%s\" (%s) : %s", namefound,
				datatype, content);
			textlist->AddItem(textline);
			delete[] datatype;
			delete[] content;

			if (typefound == B_MESSAGE_TYPE) {
				msg->FindMessage(namefound, j - 1, &a_message);
				add_message_contents(textlist, &a_message, level + 1);
			} else if (typefound == B_RAW_TYPE && strcmp(namefound,
					"_previous_") == 0) {
				if (a_message.Unflatten((const char *)voidptr) == B_OK)
					add_message_contents(textlist, &a_message, level + 1);
			}
		}
	}
}


char *
get_datatype_string(int32 type)
{
	char *str = new char[128];

	switch (type) {
		case B_ANY_TYPE:	strcpy(str, "B_ANY_TYPE"); break;
		case B_ASCII_TYPE:	strcpy(str, "B_ASCII_TYPE"); break;
		case B_BOOL_TYPE:	strcpy(str, "B_BOOL_TYPE"); break;
		case B_CHAR_TYPE:	strcpy(str, "B_CHAR_TYPE"); break;
		case B_COLOR_8_BIT_TYPE:	strcpy(str, "B_COLOR_8_BIT_TYPE"); break;
		case B_DOUBLE_TYPE:	strcpy(str, "B_DOUBLE_TYPE"); break;
		case B_FLOAT_TYPE:	strcpy(str, "B_FLOAT_TYPE"); break;
		case B_GRAYSCALE_8_BIT_TYPE:	strcpy(str, "B_GRAYSCALE_8_BIT_TYPE"); break;
		case B_INT64_TYPE:	strcpy(str, "B_INT64_TYPE"); break;
		case B_INT32_TYPE:	strcpy(str, "B_INT32_TYPE"); break;
		case B_INT16_TYPE:	strcpy(str, "B_INT16_TYPE"); break;
		case B_INT8_TYPE:	strcpy(str, "B_INT8_TYPE"); break;
		case B_MESSAGE_TYPE:	strcpy(str, "B_MESSAGE_TYPE"); break;
		case B_MESSENGER_TYPE:	strcpy(str, "B_MESSENGER_TYPE"); break;
		case B_MIME_TYPE:	strcpy(str, "B_MIME_TYPE"); break;
		case B_MONOCHROME_1_BIT_TYPE:	strcpy(str, "B_MONOCHROME_1_BIT_TYPE"); break;
		case B_OBJECT_TYPE:	strcpy(str, "B_OBJECT_TYPE"); break;
		case B_OFF_T_TYPE:	strcpy(str, "B_OFF_T_TYPE"); break;
		case B_PATTERN_TYPE:	strcpy(str, "B_PATTERN_TYPE"); break;
		case B_POINTER_TYPE:	strcpy(str, "B_POINTER_TYPE"); break;
		case B_POINT_TYPE:	strcpy(str, "B_POINT_TYPE"); break;
		case B_RAW_TYPE:	strcpy(str, "B_RAW_TYPE"); break;
		case B_RECT_TYPE:	strcpy(str, "B_RECT_TYPE"); break;
		case B_REF_TYPE:	strcpy(str, "B_REF_TYPE"); break;
		case B_RGB_32_BIT_TYPE:	strcpy(str, "B_RGB_32_BIT_TYPE"); break;
		case B_RGB_COLOR_TYPE:	strcpy(str, "B_RGB_COLOR_TYPE"); break;
		case B_SIZE_T_TYPE:	strcpy(str, "B_SIZE_T_TYPE"); break;
		case B_SSIZE_T_TYPE:	strcpy(str, "B_SSIZE_T_TYPE"); break;
		case B_STRING_TYPE:	strcpy(str, "B_STRING_TYPE"); break;
		case B_TIME_TYPE :	strcpy(str, "B_TIME_TYPE"); break;
		case B_UINT64_TYPE:	strcpy(str, "B_UINT64_TYPE"); break;
		case B_UINT32_TYPE:	strcpy(str, "B_UINT32_TYPE"); break;
		case B_UINT16_TYPE:	strcpy(str, "B_UINT16_TYPE"); break;
		case B_UINT8_TYPE:	strcpy(str, "B_UINT8_TYPE"); break;
		case B_PROPERTY_INFO_TYPE: strcpy(str, "B_PROPERTY_INFO_TYPE"); break;
		// message constants:
		case B_ABOUT_REQUESTED:	strcpy(str, "B_ABOUT_REQUESTED"); break;
		case B_WINDOW_ACTIVATED:	strcpy(str, "B_WINDOW_ACTIVATED"); break;
		case B_ARGV_RECEIVED:	strcpy(str, "B_ARGV_RECEIVED"); break;
		case B_QUIT_REQUESTED:	strcpy(str, "B_QUIT_REQUESTED"); break;
		case B_CANCEL:	strcpy(str, "B_CANCEL"); break;
		case B_KEY_DOWN:	strcpy(str, "B_KEY_DOWN"); break;
		case B_KEY_UP:	strcpy(str, "B_KEY_UP"); break;
		case B_MINIMIZE:	strcpy(str, "B_MINIMIZE"); break;
		case B_MOUSE_DOWN:	strcpy(str, "B_MOUSE_DOWN"); break;
		case B_MOUSE_MOVED:	strcpy(str, "B_MOUSE_MOVED"); break;
		case B_MOUSE_ENTER_EXIT:	strcpy(str, "B_MOUSE_ENTER_EXIT"); break;
		case B_MOUSE_UP:	strcpy(str, "B_MOUSE_UP"); break;
		case B_PULSE:	strcpy(str, "B_PULSE"); break;
		case B_READY_TO_RUN:	strcpy(str, "B_READY_TO_RUN"); break;
		case B_REFS_RECEIVED:	strcpy(str, "B_REFS_RECEIVED"); break;
		case B_SCREEN_CHANGED:	strcpy(str, "B_SCREEN_CHANGED"); break;
		case B_VALUE_CHANGED:	strcpy(str, "B_VALUE_CHANGED"); break;
		case B_VIEW_MOVED:	strcpy(str, "B_VIEW_MOVED"); break;
		case B_VIEW_RESIZED:	strcpy(str, "B_VIEW_RESIZED"); break;
		case B_WINDOW_MOVED:	strcpy(str, "B_WINDOW_MOVED"); break;
		case B_WINDOW_RESIZED:	strcpy(str, "B_WINDOW_RESIZED"); break;
		case B_WORKSPACES_CHANGED:	strcpy(str, "B_WORKSPACES_CHANGED"); break;
		case B_WORKSPACE_ACTIVATED:	strcpy(str, "B_WORKSPACE_ACTIVATED"); break;
		case B_ZOOM:	strcpy(str, "B_ZOOM"); break;
		case _APP_MENU_:	strcpy(str, "_APP_MENU_"); break;
		case _BROWSER_MENUS_:	strcpy(str, "_BROWSER_MENUS_"); break;
		case _MENU_EVENT_:	strcpy(str, "_MENU_EVENT_"); break;
		case _QUIT_:	strcpy(str, "_QUIT_"); break;
		case _VOLUME_MOUNTED_:	strcpy(str, "_VOLUME_MOUNTED_"); break;
		case _VOLUME_UNMOUNTED_:	strcpy(str, "_VOLUME_UNMOUNTED_"); break;
		case _MESSAGE_DROPPED_:	strcpy(str, "_MESSAGE_DROPPED_"); break;
		case _MENUS_DONE_:	strcpy(str, "_MENUS_DONE_"); break;
		case _SHOW_DRAG_HANDLES_:	strcpy(str, "_SHOW_DRAG_HANDLES_"); break;
		case B_SET_PROPERTY:	strcpy(str, "B_SET_PROPERTY"); break;
		case B_GET_PROPERTY:	strcpy(str, "B_GET_PROPERTY"); break;
		case B_CREATE_PROPERTY:	strcpy(str, "B_CREATE_PROPERTY"); break;
		case B_DELETE_PROPERTY:	strcpy(str, "B_DELETE_PROPERTY"); break;
		case B_COUNT_PROPERTIES:	strcpy(str, "B_COUNT_PROPERTIES"); break;
		case B_EXECUTE_PROPERTY:      strcpy(str, "B_EXECUTE_PROPERTY"); break;
		case B_GET_SUPPORTED_SUITES:	strcpy(str, "B_GET_SUPPORTED_SUITES"); break;
		case B_CUT:	strcpy(str, "B_CUT"); break;
		case B_COPY:	strcpy(str, "B_COPY"); break;
		case B_PASTE:	strcpy(str, "B_PASTE"); break;
		case B_SELECT_ALL:	strcpy(str, "B_SELECT_ALL"); break;
		case B_SAVE_REQUESTED:	strcpy(str, "B_SAVE_REQUESTED"); break;
		case B_MESSAGE_NOT_UNDERSTOOD:	strcpy(str, "B_MESSAGE_NOT_UNDERSTOOD"); break;
		case B_NO_REPLY:	strcpy(str, "B_NO_REPLY"); break;
		case B_REPLY:	strcpy(str, "B_REPLY"); break;
		case B_SIMPLE_DATA:	strcpy(str, "B_SIMPLE_DATA"); break;
		//case B_MIME_DATA	 :	strcpy(str, "B_MIME_DATA"); break;
		case B_ARCHIVED_OBJECT:	strcpy(str, "B_ARCHIVED_OBJECT"); break;
		case B_UPDATE_STATUS_BAR:	strcpy(str, "B_UPDATE_STATUS_BAR"); break;
		case B_RESET_STATUS_BAR:	strcpy(str, "B_RESET_STATUS_BAR"); break;
		case B_NODE_MONITOR:	strcpy(str, "B_NODE_MONITOR"); break;
		case B_QUERY_UPDATE:	strcpy(str, "B_QUERY_UPDATE"); break;
		case B_BAD_SCRIPT_SYNTAX: strcpy(str, "B_BAD_SCRIPT_SYNTAX"); break;

		// specifiers:
		case B_NO_SPECIFIER:	strcpy(str, "B_NO_SPECIFIER"); break;
		case B_DIRECT_SPECIFIER:	strcpy(str, "B_DIRECT_SPECIFIER"); break;
		case B_INDEX_SPECIFIER:		strcpy(str, "B_INDEX_SPECIFIER"); break;
		case B_REVERSE_INDEX_SPECIFIER:	strcpy(str, "B_REVERSE_INDEX_SPECIFIER"); break;
		case B_RANGE_SPECIFIER:	strcpy(str, "B_RANGE_SPECIFIER"); break;
		case B_REVERSE_RANGE_SPECIFIER:	strcpy(str, "B_REVERSE_RANGE_SPECIFIER"); break;
		case B_NAME_SPECIFIER:	strcpy(str, "B_NAME_SPECIFIER"); break;

		case B_ERROR:	strcpy(str, "B_ERROR"); break;

		default:	// unknown
			id_to_string(type, str);
			break;
	}
	return str;
}


char *
format_data(int32 type, char *ptr, long size)
{
	char idtext[32];
	char *str;
	float *fptr;
	double *dptr;
	entry_ref aref;
	BEntry entry;
	BPath path;
	int64 i64;
	int32 i32;
	int16 i16;
	int8 i8;
	uint64 ui64;
	uint32 ui32;
	uint16 ui16;
	uint8 ui8;
	BMessage anothermsg;
	char *tempstr;

	if (size <= 0L) {
		str = new char[1];
		*str = 0;
		return str;
	}

	switch (type) {
		case B_MIME_TYPE:
		case B_ASCII_TYPE:
		case B_STRING_TYPE:
			if (size > 512)
				size = 512;
			str = new char[size + 4];
			*str='\"';
			strncpy(str + 1, ptr, size);
			strcat(str, "\"");
			break;

		case B_POINTER_TYPE:
			str = new char[64];
			sprintf(str, "%p", *(void**)ptr);
			break;

		case B_REF_TYPE:
			str = new char[1024];
			anothermsg.AddData("myref", B_REF_TYPE, ptr, size);
			anothermsg.FindRef("myref", &aref);
			if (entry.SetTo(&aref)==B_OK){
				entry.GetPath(&path);
				strcpy(str, path.Path());
			} else
				strcpy(str, "invalid entry_ref");
			break;

		case B_SSIZE_T_TYPE:
		case B_INT64_TYPE:
			str = new char[64];
			i64 = *(int64*)ptr;
			sprintf(str, "%" B_PRId64 " (0x%" B_PRIx64 ")", i64, i64);
			break;

		case B_SIZE_T_TYPE:
		case B_INT32_TYPE:
			str = new char[64];
			i32 = *(int32*)ptr;
			sprintf(str, "%" B_PRId32 " (0x%08" B_PRId32 ")", i32, i32);
			break;

		case B_INT16_TYPE:
			str = new char[64];
			i16 = *(int16*)ptr;
			sprintf(str, "%d (0x%04X)", i16, i16);
			break;

		case B_CHAR_TYPE:
		case B_INT8_TYPE:
			str = new char[64];
			i8 = *(int8*)ptr;
			sprintf(str, "%d (0x%02X)", i8, i8);
			break;

		case B_UINT64_TYPE:
			str = new char[64];
			ui64 = *(uint64*)ptr;
			sprintf(str, "%" B_PRIu64 " (0x%" B_PRIx64 ")", ui64, ui64);
			break;

		case B_UINT32_TYPE:
			str = new char[64];
			ui32 = *(uint32*)ptr;
			sprintf(str, "%" B_PRIu32 " (0x%08" B_PRIx32 ")", ui32, ui32);
			break;

		case B_UINT16_TYPE:
			str = new char[64];
			ui16 = *(uint16*)ptr;
			sprintf(str, "%u (0x%04X)", ui16, ui16);
			break;

		case B_UINT8_TYPE:
			str = new char[64];
			ui8 = *(uint8*)ptr;
			sprintf(str, "%u (0x%02X)", ui8, ui8);
			break;

		case B_BOOL_TYPE:
			str = new char[10];
			if (*ptr)
				strcpy(str, "TRUE");
			else
				strcpy(str, "FALSE");
			break;

		case B_FLOAT_TYPE:
			str = new char[40];
			fptr = (float*)ptr;
			sprintf(str, "%.3f", *fptr);
			break;

		case B_DOUBLE_TYPE:
			str = new char[40];
			dptr = (double*)ptr;
			sprintf(str, "%.3f", *dptr);
			break;

		case B_RECT_TYPE:
			str = new char[200];
			fptr = (float*)ptr;
			sprintf(str, "BRect(%.1f, %.1f, %.1f, %.1f)", fptr[0], fptr[1],
				fptr[2], fptr[3]);
			break;

		case B_POINT_TYPE:
			str = new char[200];
			fptr = (float*)ptr;
			sprintf(str, "BPoint(%.1f, %.1f)", fptr[0], fptr[1]);
			break;

		case B_RGB_COLOR_TYPE:
			str = new char[64];
			sprintf(str, "Red=%u  Green=%u  Blue=%u  Alpha=%u",
				((uint8*)ptr)[0], ((uint8*)ptr)[1], ((uint8*)ptr)[2],
				((uint8*)ptr)[3]);
			break;

		case B_COLOR_8_BIT_TYPE:
			str = new char[size * 6 + 4];
			*str = 0;
			for (int32 i = 0; i < min_c(256, size); i++) {
				sprintf(idtext, "%u ", ((unsigned char*)ptr)[i]);
				strcat(str,idtext);
			}
			*(str+strlen(str)-2) = 0;
			break;

		case B_MESSAGE_TYPE:
			str = new char[64];
			if (anothermsg.Unflatten((const char *)ptr) == B_OK) {
				char *whatString = get_datatype_string(anothermsg.what);
				sprintf(str, "what=%s", whatString);
				delete[] whatString;
			} else
				strcpy(str, "error when unflattening");
			break;

		case B_PROPERTY_INFO_TYPE:
		{
			BPropertyInfo propinfo;
			if (propinfo.Unflatten(B_PROPERTY_INFO_TYPE, (const void *)ptr,
					size) == B_OK) {
				str = new char[size * 32];	// an approximation

				const property_info *pinfo = propinfo.Properties();

				sprintf(str, "\n        property   commands                            "
					"specifiers              types\n-----------------------------------"
					"----------------------------------------------------------------\n");
				for (int32 pinfo_index = 0; pinfo_index < propinfo.CountProperties(); pinfo_index++) {
					strcat(str,  "                "
						+ (strlen(pinfo[pinfo_index].name) < 16 ?
							strlen(pinfo[pinfo_index].name) : 16));
					strcat(str, pinfo[pinfo_index].name);
					strcat(str, "   ");
					char *start = str + strlen(str);

					for (int32 i = 0; i < 10 && pinfo[pinfo_index].commands[i];
							i++) {
						tempstr = get_datatype_string(
							pinfo[pinfo_index].commands[i]);
						strcat(str, tempstr);
						strcat(str, " ");
						delete[] tempstr;
					}

					// pad the rest with spaces
					if (strlen(start) < 36) {
						strcat(str, "                                    "
							+ strlen(start));
					} else
						strcat(str, "  " );

					for (int32 i = 0; i < 10 && pinfo[pinfo_index].specifiers[i]; i++) {
						switch (pinfo[pinfo_index].specifiers[i]) {
							case B_NO_SPECIFIER:
								strcat(str, "NONE ");
								break;
							case B_DIRECT_SPECIFIER:
								strcat(str, "DIRECT ");
								break;
							case B_INDEX_SPECIFIER:
								strcat(str, "INDEX ");
								break;
							case B_REVERSE_INDEX_SPECIFIER:
								strcat(str, "REV.INDEX ");
								break;
							case B_RANGE_SPECIFIER:
								strcat(str, "RANGE ");
								break;
							case B_REVERSE_RANGE_SPECIFIER:
								strcat(str, "REV.RANGE ");
								break;
							case B_NAME_SPECIFIER:
								strcat(str, "NAME ");
								break;
							case B_ID_SPECIFIER:
								strcat(str, "ID ");
								break;
							default:
								strcat(str, "<NONE> ");
								break;
							}
						}

						// pad the rest with spaces
						if (strlen(start) < 60) {
							strcat(str, "                                      "
								"                      " + strlen(start));
						} else
							strcat(str, "  ");
						for (int32 i = 0; i < 10
								&& pinfo[pinfo_index].types[i] != 0; i++) {
							uint32 type = pinfo[pinfo_index].types[i];
							char str2[6];
							snprintf(str2, sizeof(str2), "%c%c%c%c ",
								int(type & 0xFF000000) >> 24,
								int(type & 0xFF0000) >> 16,
								int(type & 0xFF00) >> 8,
								(int)type & 0xFF);
							strcat(str, str2);
						}

						for (int32 i = 0; i < 3; i++) {
							for (int32 j = 0; j < 5
									&& pinfo[pinfo_index].ctypes[i].pairs[j].type
										!= 0; j++) {
								uint32 type = pinfo[pinfo_index].ctypes[i].pairs[j].type;
								char str2[strlen(pinfo[pinfo_index].ctypes[i].pairs[j].name) + 8];
								snprintf(str2, sizeof(str2),
									"(%s %c%c%c%c)",
									pinfo[pinfo_index].ctypes[i].pairs[j].name,
									int(type & 0xFF000000) >> 24,
									int(type & 0xFF0000) >> 16,
									int(type & 0xFF00) >> 8,
									(int)type & 0xFF);
								strcat(str, str2);
							}
						}
						strcat(str, "\n");

						// is there usage info?
						if (pinfo[pinfo_index].usage) {
							strcat(str, "                   Usage: ");
							strcat(str, pinfo[pinfo_index].usage);
							strcat(str, "\n");
						}
					}


					// handle value infos....
				const value_info *vinfo = propinfo.Values();
				int32 vinfo_count = propinfo.CountValues();
#if TEST_VALUEINFO>0
				value_info vinfo[10] = {
					{"Backup", 'back', B_COMMAND_KIND,
						"This command backs up your hard drive."},
					{"Abort", 'abor', B_COMMAND_KIND,
						"Stops the current operation..."},
					{"Type Code", 'type', B_TYPE_CODE_KIND,
						"Type code info..."}
				};
				vinfo_count = 3;
#endif

				if (vinfo && vinfo_count > 0) {
					sprintf(str + strlen(str),
						"\n            name   value                            "
						"   kind\n---------------------------------------------"
						"-----------------------------------\n");

					for (int32 vinfo_index = 0; vinfo_index < vinfo_count;
						vinfo_index++) {
						char *start = str + strlen(str);
						strcat(str, "                " + (strlen(vinfo[vinfo_index].name) < 16 ? strlen(vinfo[vinfo_index].name) : 16));
						strcat(str, vinfo[vinfo_index].name);
						strcat(str, "   ");

						sprintf(str + strlen(str), "0x%8" B_PRIx32 " (",
							vinfo[vinfo_index].value);
						id_to_string(vinfo[vinfo_index].value, str + strlen(str));
						strcat(str, ")");

							// pad the rest with spaces
						if (strlen(start) < 36 + 19) {
							strcat(str, "                                      "
								"                 " + strlen(start));
						} else
							strcat(str, "  ");

						switch (vinfo[vinfo_index].kind) {
							case B_COMMAND_KIND:
								strcat(str, "COMMAND         ");
								break;

							case B_TYPE_CODE_KIND:
								strcat(str, "TYPE CODE       ");
								break;

							default:
								strcat(str, "unknown         ");
								break;
						}

						strcat(str, "\n");

							// is there usage info?
						if (vinfo[vinfo_index].usage) {
							strcat(str, "                   Usage: ");
							strcat(str, vinfo[vinfo_index].usage);
							strcat(str, "\n");
						}
					}
				}
			} else {
				str = new char[64];
				strcpy(str, "error when unflattening");
			}
			break;
		}

		default:
			str = new char[min_c(256, size) * 20 + 4];
			*str = 0;
			for (int32 i = 0; i < min_c(256, size); i++) {
				sprintf(idtext, "0x%02X, ", (uint16)ptr[i]);
				strcat(str, idtext);
			}
			*(str + strlen(str) - 2) = 0;
			break;
	}

	return str;
}


char *
id_to_string(long ID, char *here)
{
	uint8 digit0 = (ID>>24)& 255;
	uint8 digit1 = (ID>>16)& 255;
	uint8 digit2 = (ID>>8) & 255;
	uint8 digit3 = (ID) & 255;
	bool itsvalid = false;

	if (digit0 == 0) {
		if (digit1 == 0) {
			if (digit2 == 0) {
				// 1 digits
				itsvalid = is_valid_char(digit3);
				sprintf(here, "'%c'", digit3);
			} else {
				// 2 digits
				itsvalid = is_valid_char(digit2) && is_valid_char(digit3);
				sprintf(here, "'%c%c'", digit2, digit3);
			}
		} else {
			// 3 digits
			itsvalid = is_valid_char(digit1) && is_valid_char(digit2)
				&& is_valid_char(digit3);
			sprintf(here, "'%c%c%c'", digit1, digit2, digit3);
		}
	} else {
		// 4 digits
		itsvalid = is_valid_char(digit0) && is_valid_char(digit1)
			&& is_valid_char(digit2) && is_valid_char(digit3);
		sprintf(here, "'%c%c%c%c'", digit0, digit1, digit2, digit3);
	}

	if (!itsvalid)
		sprintf(here, "%ldL", ID);

	return here;
}


bool
is_valid_char(uint8 c)
{
	return c >= 32 && c < 128;
}

