/*===============================================================
** Clipboard - Applet - Code File
**===============================================================
** 24 June 2002 - Mathew Hounsell - Created
*/
#include "be/app/Clipboard.h"

#include <stdio.h>
#include <string.h>

#include <new.h>

/*===============================================================
** Status
*/
namespace Status {

	status_t Code = B_OK;
}

/*===============================================================
** Clipboard
*/
namespace Clipboard {

	// Use system clipboard by default.
	char const * Name = "system";
	bool InstanceExists = false;

    //-----------------------------------------------------------
	class Locker {
		private:	BClipboard *clip;
		public:		Locker( BClipboard * nclip )
						: clip( nclip )
					{ 
						if( ! ( InstanceExists = clip->Lock() ) )
							Status::Code = B_NAME_NOT_FOUND ;
					}
		public:		~Locker( void )
						{  clip->Unlock();  }
	};

    //-----------------------------------------------------------
	BClipboard * Instance = NULL;

	// Open or Create clipboard.
	bool ConnectOrCreate( void ) {
		Instance = new( std::nothrow ) BClipboard( Name );
		if( ! ( InstanceExists = ( Instance != NULL ) ) ) {
			Status::Code = B_NO_MEMORY;
		}
		return InstanceExists;
	}
	
    //-----------------------------------------------------------
	bool Print( void ) {
		// Retrieve Data - Concurrent Access Possible.
		Locker clip_lock( Instance ); // Unlocks on destruction.
		if( InstanceExists ) {
			BMessage *data = NULL;
			if( 0 == ( data = Instance->Data() ) ) {
				Status::Code = B_NO_INIT;
			} else {
				// Clipboard fills in message
				// Then tell message to print itself.
				data->PrintToStream();
			}
		}
	}

    //-----------------------------------------------------------
	bool Read( char const * entry_name ) {
		// Retrieve Data - Concurrent Access Possible.
		Locker clip_lock( Instance ); // Unlocks on destruction.
		if( InstanceExists ) {
			BMessage *data = NULL;
			if( 0 == ( data = Instance->Data() ) ) {
				Status::Code = B_NO_INIT;
			} else {
				type_code entry_type = 0;
				int32 entry_count = 0;
	
				if( B_OK == ( Status::Code = data->GetInfo( entry_name, & entry_type, & entry_count ) ) ) {
					ssize_t entry_size = 0;
					void const * entry_data = 0;
	
					if( B_OK == ( Status::Code = data->FindData( entry_name, entry_type, & entry_data, & entry_size ) ) ) {
						fwrite( entry_data, entry_size, 1, stdout );
					}
				}
			}
		}
	}
}

/*===============================================================
** Arguments
*/
namespace Arguments {
	enum { NONE, PRINT, READ, WRITE }  Operation;
	char const * ProgramName = "";

    //-----------------------------------------------------------
	void Usage( void )
	{
		printf( "USAGE : %s -p [clipboard-name]\n", ProgramName );
		printf( "USAGE : %s -r [entry] [clipboard-name]\n", ProgramName );
		//	printf( "USAGE : %s -w [entry] [clipboard-name]\n", ProgramName );
	}

    //-----------------------------------------------------------
	void Decode( int const argc, char const * const * const argv )
	{
		ProgramName = argv[0];
		Operation = NONE;

		// Test arguments.
		if( argc < 2 || argc > 3 ) {
			Usage();
		} else if( argv[1][0] != '-' ) {
			Usage();
		} else if( argv[1][1] == 'p' && ( argc == 2 || argc == 3 ) ) {
			Operation = PRINT;
		} else if( argv[1][1] == 'r' && ( argc == 3 || argc == 4 ) ) {
			Operation = READ;
	//	} else if( argv[1][1] == 'w' && ( argc == 3 || argc == 4 ) ) {
	//		op = WRITE;
		} else {
			Usage();
		}

		if( Operation == PRINT && argc == 3 ) {
			// Use user clipboard.
			Clipboard::Name = argv[2];
		} else if( ( Operation == READ || Operation == WRITE ) && argc == 4 ) {
			// Use user clipboard.
			Clipboard::Name = argv[3];
		}
	}
}

/*===============================================================
** Main Routine
*/
int main( int argc, char **argv )
{
	Arguments::Decode( argc, argv );

	if( Arguments::NONE == Arguments::Operation ) {
		Status::Code = B_BAD_VALUE;
	} else {
		// Open or Create clipboard.
		if( Clipboard::ConnectOrCreate() ) {
			switch( Arguments::Operation ) {
				case Arguments::PRINT:
					Clipboard::Print();
					break;
				case Arguments::READ:
					Clipboard::Read( argv[2] );
					break;
			}
		}
	}
	
	if( B_OK != Status::Code ) {
		fputs( strerror( Status::Code ), stderr );
		fputc( '\n', stderr );
		return Status::Code;
	}
	return 0;
}

/*===============================================================
** End Of Code
*/
