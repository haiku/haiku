/*********************************************************************
** Roster Applet
**********************************************************************
** 2002-04-30  Mathew Hounsell  Created.
*/

#include <Roster.h>

#include <Path.h>
#include <Entry.h>

#include <List.h>

#include <iostream>
#include <iomanip>

#include <cstring>
#include <cstdlib>
#include <cassert>

/*********************************************************************
*/
bool OutputTeam( void * item_p )
{
	static uint32 i = 0u; // counter

	app_info info;
	app_info *pinfo = &info;

	// Roster stores team_id not team_id * 
	team_id id = reinterpret_cast<team_id>( item_p );

	// Get info on team
	if( be_roster->GetRunningAppInfo( id, &info ) == B_OK ) {

		// Allocate a entry and get it's path.
		// - works as they are independant (?)
		BEntry entry( & info.ref );
		BPath path( & entry );
		
		// Output - leave the work to ostream
		std::cout
			<< setw(2) << i << ' '
// short	<< setw(32) << info.ref.name << ' '
			<< setw(32) << path.Path() << ' '
			<< setw(6) << info.thread << ' '
			<< setw(4) << info.team << ' '
			<< setw(4) << info.port << ' '
			<< setw(4)
		;
		// Go hex, output, the go dec
		hex( std::cout );
		std::cout << info.flags;
		dec( std::cout );
	
		// Output a signture
		std::cout
			<< " ("	<< info.signature << ')'
			<< std::endl
		;
	
 		/* NOW */ ++i;
 	}
 	return false;
}
 
/*********************************************************************
*/
int main( int argc, char ** argv )
{
	// Don't have an BApplication as it is a waste.
	
	// Access Roster
	// - address automatically assigned to be_roster as it's a singleton
	new BRoster();
	
	cout
		<< "                               path thread team port flags\n"
		   "-- -------------------------------- ------ ---- ---- -----"
		<< std::endl
	;

	// Retriev the running list.
	BList applist( 0 );
	be_roster->GetAppList( & applist );

	// Iterate through the list.
	// void DoForEach(bool (*func)(void *) )
	applist.DoForEach( OutputTeam );
	
	return 0;
}
