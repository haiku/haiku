// RosterShell.cpp

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <iostream>

#include <List.h>
#include <Path.h>
#include <Roster.h>

using std::string;
using std::vector;

const char kShellUsage[] =
"Commands       Parameters     Description\n"
"---------------------------------------------------------------------------\n"
"activate, a    [ <team> ]     activates the application specified by <team>\n"
"exit, e                       exits the roster shell\n"
"help, h                       prints this help\n"
"launch         [ <program> ]  Executes <program> via BRoster::Launch()\n"
"list, l        [ <teams> ]    lists the applications specified by <teams>\n"
"                              or all registered applications\n"
"list-long, ll  [ <teams> ]    lists the applications specified by <teams>\n"
"                              or all registered applications (long format)\n"
"quit, q        [ <teams> ]    quits the applications specified by <teams>\n"
"\n"
;

class Shell {
public:
	Shell()
		:
		fTerminating(false)
	{
	}

	bool IsTerminating() const { return fTerminating; }

	void
	DoCommand(vector<string>& cmdLine)
	{
		if (cmdLine.size() > 0) {
			string command = cmdLine[0];
			if (command == "activate" || command == "a")
				CmdActivate(cmdLine);
			else if (command == "exit" || command == "e")
				CmdExit(cmdLine);
			else if (command == "help" || command == "h")
				Usage(string(" "));
			else if (command == "launch")
				CmdLaunch(cmdLine);
			else if (command == "list" || command == "l")
				CmdList(cmdLine);
			else if (command == "list-long" || command == "ll")
				CmdListLong(cmdLine);
			else if (command == "quit" || command == "q")
				CmdQuit(cmdLine);
			else
				Usage(string("Unknown command `") + command + "'");
		}
	}

	void
	Usage(string error)
	{
		if (error.length() > 0)
			std::cout << error << std::endl;
		std::cout << kShellUsage;
	}

	void
	CmdActivate(vector<string>& args)
	{
		BRoster roster;

		// get a list of team IDs
		BList teamList;
		if (args.size() <= 1) {
			printf("activate: requires exactly one argument\n");
			return;
		}

		ParseTeamList(args, &teamList);
		int32 count = teamList.CountItems();
		if (count != 1) {
			printf("activate: requires exactly one argument\n");
			return;
		}

		// activate the team
		team_id team = (team_id)(uintptr_t)teamList.ItemAt(0);
		status_t error = roster.ActivateApp(team);
		if (error != B_OK) {
			printf("activate: failed to activate application %" B_PRId32
				": %s\n", team, strerror(error));
		}
	}

	void
	CmdExit(vector<string>&)
	{
		fTerminating = true;
	}

	void
	CmdLaunch(vector<string>& args)
	{
		// check args
		if (args.size() != 2) {
			printf("launch: requires exactly one argument\n");
			return;
		}

		string program = args[1];

		// get an app ref
		entry_ref ref;
		status_t error = get_ref_for_path(program.c_str(), &ref);
		if (error != B_OK) {
			printf("launch: Failed to get entry ref for \"%s\": %s\n",
				program.c_str(), strerror(error));
			return;
		}

		// launch the app
		BRoster roster;
		team_id teamID;
		error = roster.Launch(&ref, (const BMessage*)NULL, &teamID);
		if (error == B_OK) {
			printf("launched \"%s\", team id: %" B_PRId32 "\n", program.c_str(),
				teamID);
		} else {
			printf("launch: Failed to launch \"%s\": %s\n",
				program.c_str(), strerror(error));
		}
	}

	static void
	ParseTeamList(vector<string>& args, BList* teamList)
	{
		for (int32 i = 1; i < (int32)args.size(); i++) {
			string arg = args[i];
			team_id team = -1;
			if (sscanf(arg.c_str(), "%" B_PRId32, &team) > 0)
				teamList->AddItem((void*)(addr_t)team);
		}
	}

	void
	CmdList(vector<string>& args)
	{
		BRoster roster;

		// get a list of team IDs
		BList teamList;
		if (args.size() > 1)
			ParseTeamList(args, &teamList);
		else
			roster.GetAppList(&teamList);

		// print an info for each team
		int32 count = teamList.CountItems();
		printf("%-8s%-40s\n", "team", "signature");
		printf("---------------------------------------------------------\n");
		for (int32 i = 0; i < count; i++) {
			team_id team = (team_id)(uintptr_t)teamList.ItemAt(i);
			app_info info;
			status_t error = roster.GetRunningAppInfo(team, &info);
			if (error == B_OK)
				printf("%-8" B_PRId32 "%-40s\n", team, info.signature);
			else {
				printf("%-8" B_PRId32 "failed to get the app_info: %s\n", team,
					   strerror(error));
			}
		}
		printf("\n");
	}

	void
	CmdListLong(vector<string>& args)
	{
		BRoster roster;

		// get a list of team IDs
		BList teamList;
		if (args.size() > 1)
			ParseTeamList(args, &teamList);
		else
			roster.GetAppList(&teamList);

		// print an info for each team
		int32 count = teamList.CountItems();
		for (int32 i = 0; i < count; i++) {
			team_id team = (team_id)(uintptr_t)teamList.ItemAt(i);
			printf("team %8" B_PRId32 "\n", team);
			printf("-------------\n");
			app_info info;
			status_t error = roster.GetRunningAppInfo(team, &info);
			if (error == B_OK) {
				printf("signature: %s\n", info.signature);
				printf("thread:    %" B_PRId32 "\n", info.thread);
				printf("port:      %" B_PRId32 "\n", info.port);
				printf("flags:     0x%" B_PRIx32 "\n", info.flags);
				printf("file:      %s\n", BPath(&info.ref).Path());
			} else
				printf("failed to get the app_info: %s\n", strerror(error));
			printf("\n");
		}
	}

	void
	CmdQuit(vector<string>& args)
	{
		BRoster roster;
		// get a list of team IDs
		BList teamList;
		if (args.size() <= 1) {
			printf("quit: requires at least one argument\n");
			return;
		}

		ParseTeamList(args, &teamList);
		int32 count = teamList.CountItems();
		if (count < 1) {
			printf("quit: requires at least one argument\n");
			return;
		}

		// send a B_QUIT_REQUESTED message to each team
		for (int32 i = 0; i < count; i++) {
			team_id team = (team_id)(uintptr_t)teamList.ItemAt(i);
			status_t error = B_OK;
			BMessenger messenger(NULL, team, &error);
			if (messenger.IsValid()) {
				error = messenger.SendMessage(B_QUIT_REQUESTED);
				if (error != B_OK) {
					printf("quit: failed to deliver the B_QUIT_REQUESTED "
						   "message to team %" B_PRId32 "\n", team);
					printf("      %s\n", strerror(error));
				}
			} else {
				printf("quit: failed to create a messenger for team %" B_PRId32
					"\n", team);
				printf("      %s\n", strerror(error));
			}
		}
	}

private:
	bool	fTerminating;
};


// read_line
bool
read_line(char* line, size_t size)
{
	std::cout << "roster> ";
	std::cout.flush();

	try {
		std::cin.getline(line, size);
	} catch (const std::exception &ex) {
		return false;
	}

	return true;
}

// main
int
main()
{
	Shell shell;

	// main input loop
	char line[10240];
	while (!shell.IsTerminating() && read_line(line, sizeof(line))) {
		// parse args
		std::istringstream in(line);
		vector<string> args;
		string arg;
		while (in >> arg)
			args.push_back(arg);
		// invoke command
		shell.DoCommand(args);
	}

	return 0;
}

