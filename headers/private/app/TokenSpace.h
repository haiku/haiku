//------------------------------------------------------------------------------
//	TokenSpace.h
//
//------------------------------------------------------------------------------

#ifndef TOKENSPACE_H
#define TOKENSPACE_H

// Standard Includes -----------------------------------------------------------
#include <map>
#include <stack>

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Locker.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define B_NULL_TOKEN		-1
#define B_ANY_TOKEN			0
#define B_HANDLER_TOKEN		1

// Globals ---------------------------------------------------------------------

namespace BPrivate {

typedef void (*new_token_callback)(int16, void*);
typedef void (*remove_token_callback)(int16, void*);
typedef bool (*get_token_callback)(int16, void*);

class BTokenSpace
{
	public:
		BTokenSpace();
		~BTokenSpace();

		int32		NewToken(int16 type, void* object,
							 new_token_callback callback= NULL);
		bool		RemoveToken(int32 token, remove_token_callback callback = NULL);
		bool		CheckToken(int32, int16) const;
		status_t	GetToken(int32, int16, void**,
							 get_token_callback callback = NULL) const;

// Possible expansion
//		void Dump(BDataIO&, bool) const;
//		int32 NewToken(void*, BDirectMessageTarget*, void (*)(short, void*));
//		bool SetTokenTarget(uint32, BDirectMessageTarget*);
//		BDirectMessageTarget* TokenTarget(uint32 token, int16 type);

	private:
		struct TTokenInfo
		{
			int16 type;
			void* object;
		};

		typedef std::map<int32, TTokenInfo>	TTokenMap;

		TTokenMap			fTokenMap;
		std::stack<int32>	fTokenBin;
		int32				fTokenCount;
		BLocker				fLocker;
};

// Possible expansion
//_delete_tokens_();
//_init_tokens_();
//get_handler_token(short, void*);
//get_token_list(long, long*);
//new_handler_token(short, void*);
//remove_handler_token(short, void*);

extern _IMPEXP_BE BTokenSpace gDefaultTokens;

}	// namespace BPrivate

#endif	//TOKENSPACE_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

