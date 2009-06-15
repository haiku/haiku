/*
 * Copyright 2009, Philippe Houdoin, phoudoin@haiku-os.org. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "hdb.h"

#include "RunningTeamsWindow.h"
#include "TeamWindow.h"

#include <Application.h>
#include <Alert.h>
#include <TextView.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include <stdio.h>
#include <string.h>

const char* kSignature = "application/x-vnd.Haiku-Debugger";

class Debugger : public BApplication {
    public:
                        Debugger();
        virtual			~Debugger();

        virtual	void	ReadyToRun();

        virtual	void	RefsReceived(BMessage *message);
        virtual	void	ArgvReceived(int32 argc, char **argv);
        virtual	void	MessageReceived(BMessage *message);

        virtual	void	AboutRequested();

        static	void	ShowAbout();

    private:
        status_t		Debug(team_id team);
        status_t		Debug(BEntry &entry);

        RunningTeamsWindow * 	fRunningTeamsWindow;
};


Debugger::Debugger()
    : BApplication(kSignature),
    fRunningTeamsWindow(NULL)
{
}


Debugger::~Debugger()
{
}


status_t
Debugger::Debug(BEntry &entry)
{
    // Does the file really exist?
    if (!entry.Exists())
        return B_ENTRY_NOT_FOUND;

    // TODO
    BWindow *window = new TeamWindow(0); // team);
    window->Show();

    return B_OK;
}


status_t
Debugger::Debug(team_id team)
{
    // int32 teamWindows = 0;

    // Do we already have that window open?
    for (int32 i = CountWindows(); i-- > 0; ) {
        TeamWindow *window = dynamic_cast<TeamWindow *>(WindowAt(i));
        if (window == NULL)
            continue;

        if (window->Team() == team) {
            window->Activate(true);
            return B_OK;
        }
        // teamWindows++;
    }

    BWindow *window = new TeamWindow(team);
    window->Show();

    return B_OK;
}


void
Debugger::ReadyToRun()
{
    // are there already windows open?
    if (CountWindows() != 0)
        return;

    // if not, open the running teams window
    PostMessage(kMsgOpenRunningTeamsWindow);
}


void
Debugger::RefsReceived(BMessage *message)
{
    int32 index = 0;
    entry_ref ref;
    while (message->FindRef("refs", index++, &ref) == B_OK) {
        BEntry entry;
        status_t status = entry.SetTo(&ref);

        if (status == B_OK)
            status = Debug(entry);

        if (status != B_OK) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer),
                "Could not debug \"%s\":\n"
                "%s",
                ref.name, strerror(status));

            (new BAlert("Debugger request",
                buffer, "Ok", NULL, NULL,
                B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
        }
    }
}


void
Debugger::ArgvReceived(int32 argc, char **argv)
{
    BMessage *message = CurrentMessage();

    BDirectory currentDirectory;
    if (message)
        currentDirectory.SetTo(message->FindString("cwd"));

    BMessage refs;

    for (int i = 1 ; i < argc ; i++) {
        BPath path;
        if (argv[i][0] == '/')
            path.SetTo(argv[i]);
        else
            path.SetTo(&currentDirectory, argv[i]);

        status_t status;
        entry_ref ref;
        BEntry entry;

        if ((status = entry.SetTo(path.Path(), false)) != B_OK
            || (status = entry.GetRef(&ref)) != B_OK) {
            fprintf(stderr, "Could not open file \"%s\": %s\n", path.Path(), strerror(status));
            continue;
        }

        refs.AddRef("refs", &ref);
    }

    RefsReceived(&refs);
}


void
Debugger::MessageReceived(BMessage *message)
{
    switch (message->what) {
        case kMsgOpenRunningTeamsWindow:
            if (fRunningTeamsWindow == NULL) {
                fRunningTeamsWindow = new RunningTeamsWindow();
                fRunningTeamsWindow->Show();
            } else
                fRunningTeamsWindow->Activate(true);
            break;

        case kMsgRunningTeamsWindowClosed:
            fRunningTeamsWindow = NULL;
            // supposed to fall through
        case kMsgWindowClosed:
            if (CountWindows() == 1)
                // Last window is being closed: quit application
                PostMessage(B_QUIT_REQUESTED);
            break;

        case kMsgOpenTeamWindow:
        {
            team_id team;
            if (message->FindInt32("team_id", &team) == B_OK)
                Debug(team);
            break;
        }

        default:
            BApplication::MessageReceived(message);
            break;
    }
}


void
Debugger::AboutRequested()
{
    ShowAbout();
}


/*static*/ void
Debugger::ShowAbout()
{
    BAlert *alert = new BAlert("about", "Debugger\n"
        "\twritten by Philippe Houdoin\n"
        "\tCopyright 2009, Haiku Inc.\n", "Ok");
    BTextView *view = alert->TextView();
    BFont font;

    view->SetStylable(true);

    view->GetFont(&font);
    font.SetSize(18);
    font.SetFace(B_BOLD_FACE);
    view->SetFontAndColor(0, 15, &font);

    alert->Go();
}


//	#pragma mark -


int
main(int /* argc */, char ** /* argv */)
{
    Debugger app;
    app.Run();

    return 0;
}
