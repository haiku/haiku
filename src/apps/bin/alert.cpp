/*********************************************************************
** Alert Applet
**********************************************************************
** 2002-04-28  Mathew Hounsell  Created.
*/

#include <Application.h>
#include <Alert.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

/*********************************************************************
*/

// #define SIGNATURE "application/x-vnd.Be-cmd-alert"
#define SIGNATURE "application/x-vnd.OBOS.cmd-alert"
#define APP_NAME "alert"

#define BUTTON_DEFAULT "OK"

#define ERR_INIT_FAIL  127
#define ERR_ARGS_FAIL  126
#define ERR_NONE       0

/*********************************************************************
*/
class Arguments {
	bool ok, recved, modal;
	alert_type icon;
	char *arg_text, *arg_but0, *arg_but1, *arg_but2;
	char *choice_text;
	int32 choice;
public:
	Arguments( void ) {
		ok = recved = modal = false;
		icon = B_INFO_ALERT;
		arg_text = arg_but0 = arg_but1 = arg_but2 = 0;
		choice = 0;
	}
	~Arguments( void ) {
		std::free( arg_text );
		std::free( arg_but0 );
		std::free( arg_but1 );
		std::free( arg_but2 );
	}
	
	inline bool Good( void ) const { return ok; }
	inline bool Bad( void ) const { return ! ok; }

	void ArgvReceived( int32 argc, char **argv )
	{
		recved = true;
	
		if( argc < 2 ) return;

		int32 start = 1;
		bool flag_icon = false;
		
		// Look for '--' options'		
		for( int32 i = 1; i < argc; ++i, ++start ) {
			if( '-' == argv[i][0] && '-' == argv[i][1] ) {
				if( 0 == strcmp( argv[i] + 2, "modal" ) ) {
					modal = true;
//				} else if( 0 == strcmp( argv[i] + 2, "empty" )
//				       ||  0 == strcmp( argv[i] + 2, "none" ) ) {
				} else if( 0 == strcmp( argv[i] + 2, "empty" ) ) {
					if( flag_icon ) return;
					icon = B_EMPTY_ALERT;
					flag_icon = true;
				} else if( 0 == strcmp( argv[i] + 2, "info" ) ) {
					if( flag_icon ) return;
					icon = B_INFO_ALERT;
					flag_icon = true;
				} else if( 0 == strcmp( argv[i] + 2, "idea" ) ) {
					if( flag_icon ) return;
					icon = B_IDEA_ALERT;
					flag_icon = true;
//				} else if( 0 == strcmp( argv[i] + 2, "warning" )
//				       ||  0 == strcmp( argv[i] + 2, "warn" ) ) {
				} else if( 0 == strcmp( argv[i] + 2, "warning" ) ) {
					if( flag_icon ) return;
					icon = B_WARNING_ALERT;
					flag_icon = true;
				} else if( 0 == strcmp( argv[i] + 2, "stop" ) ) {
					if( flag_icon ) return;
					icon = B_STOP_ALERT;
					flag_icon = true;
				} else {
					return;
				}
			} else break;
		}
		
		if( start >= argc ) { // Nothing but --modal
			return;
		}
		
		arg_text = strdup( argv[start++] );
		if( start < argc ) arg_but0 = strdup( argv[start++] );
		else arg_but0 = strdup( BUTTON_DEFAULT );
		if( start < argc ) arg_but1 = strdup( argv[start++] );
		if( start < argc ) arg_but2 = strdup( argv[start++] );
		
		ok = true;
	}

	inline char const * Title( void ) const { return APP_NAME; }

	inline char const * Text( void ) const { return arg_text; }
	inline char const * ButtonZero( void ) const { return arg_but0; }
	inline char const * ButtonOne( void ) const { return arg_but1; }
	inline char const * ButtonTwo( void ) const { return arg_but2; }
	
	inline alert_type Type( void ) const { return icon; }

	inline bool IsModal( void ) const { return modal; }

	inline bool Received( void ) const { return recved; }

	inline int32 ChoiceNumber( void ) const { return choice; }
	inline char const * ChoiceText( void ) const { return choice_text; }

	void SetChoice( int32 but )
	{
		assert( but >= 0 && but <= 2 );
		
		choice = but;
		switch( choice ) {
			case 0: choice_text = arg_but0; break;
			case 1: choice_text = arg_but1; break;
			case 2: choice_text = arg_but2; break;
		}
	}

	void Usage( void )
	{
		fprintf(
		    stderr,
			"usage: " APP_NAME " <type> ] [ --modal ] [ --help ] text [ button1 [ button2 [ button3 ]]]\n"
			  "<type> is --empty | --info | --idea | --warning | --stop\n"
			  "--modal makes the alert system modal and shows it in all workspaces.\n"
			  "If any button argument is given, exit status is button number (starting with 0)\n"
			  " and '" APP_NAME "' will print the title of the button pressed to stdout.\n"
		);
	} 

} arguments;

/*********************************************************************
*/
class AlertApplication : public BApplication {
	typedef BApplication super;

public:
	AlertApplication( void ) : super( SIGNATURE ) { }
	// No destructor as it crashes app.

	virtual void ArgvReceived( int32 argc, char **argv )
	{
		if( ! arguments.Received() ) {
			arguments.ArgvReceived( argc, argv );
		}
	}

	// Prepare to Run
	virtual void ReadyToRun(void) {

		if( arguments.Good() ) {
			BAlert * alert_p
				= new BAlert(
						arguments.Title(),
						arguments.Text(),
						arguments.ButtonZero(),
						arguments.ButtonOne(),
						arguments.ButtonTwo(),
						B_WIDTH_AS_USUAL,
						arguments.Type()
					);
	
			if( arguments.IsModal() ) {
				alert_p->SetFeel( B_MODAL_ALL_WINDOW_FEEL );
			}
			
			arguments.SetChoice( alert_p->Go() );
		}
				
		Quit();
	}
};

/*********************************************************************
** Acording to the BeBook youm should use the message handling in
** BApplication for arguments. However that requires the app to
** be created.
*/
int main( int argc, char ** argv )
{
	arguments.ArgvReceived( int32(argc), argv );
	if( ! arguments.Good() ) {
		arguments.Usage();
		return ERR_ARGS_FAIL;
	}

	int errorlevel = ERR_NONE;
	
	// App is Automatically assigned to be_app
	new AlertApplication();

	if( B_OK != be_app->InitCheck() ) {
		// Cast for the right version.
		errorlevel = ERR_INIT_FAIL;
	} else {
		// Run
		be_app->Run();
		if( ! arguments.Good() ) { // In case of Roster start.
			arguments.Usage();
			errorlevel = ERR_ARGS_FAIL;
		} else {
			fprintf( stdout, "%s\n", arguments.ChoiceText() );
			errorlevel = arguments.ChoiceNumber();
		}
	}

	return errorlevel;
}