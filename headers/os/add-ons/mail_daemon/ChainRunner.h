#ifndef ZOIDBERG_MAIL_CHAIN_RUNNER_H
#define ZOIDBERG_MAIL_CHAIN_RUNNER_H
/* ChainRunner - runs the mail inbound and outbound chains
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <MailAddon.h>
#include <MailSettings.h>
#include <Looper.h>
#include <status.h>

class BStringList;
class BMailStatusWindow;
class BMailChain;

class BMailChainCallback {
	public:
		virtual void Callback(status_t result) = 0;
		// Called by the callback routines in ChainRunner
		// result is the message that ended the chain
		// (MD_HANDLED, MD_DISCARD, MD_NO_MORE_MESSAGES)
		// Obviously, for process callbacks, result is
		// gauranteed to be MD_NO_MORE_MESSAGES.
};

class BMailChainRunner : public BLooper {
	public:
		BMailChainRunner(BMailChain *chain, BMailStatusWindow *status,
			bool self_destruct_when_done = true, bool save_chain_when_done = false,
			bool destruct_chain_when_done = false);
		~BMailChainRunner();

		//----Callback functions. Callback objects will be deleted for you.
		
		void RegisterMessageCallback(BMailChainCallback *callback);
		// Your callback->Callback() function will be called when
		// the current message is done being processed.
		
		void RegisterProcessCallback(BMailChainCallback *callback);
		// Your callback->Callback() function will be called when
		// a filter returns MD_PASS_COMPLETE and before we go back
		// to waiting for new messages, or when the fetch list
		// runs out.
		
		void RegisterChainCallback(BMailChainCallback *callback);
		// Your callback->Callback() function will be called when
		// a filter returns MD_ALL_PASSES_DONE and before everything
		// is unloaded and sent home.
		
		void Stop();
		void ReportProgress(int bytes, int messages, const char *message = NULL);
		void ResetProgress(const char *message = NULL);

		void GetMessages(BStringList *list, int32 bytes);
		void GetSingleMessage(const char *uid, int32 length, BPath *into);
		// The bytes or length field is the total size, used for updating the
		// progress bar.  Use -1 for unknown maximum size.

		bool QuitRequested();
		
		void ShowError(const char *error);
		void ShowMessage(const char *message);

		BMailChain *Chain();
		
		//----The big, bad asynchronous RunChain() function. Pretty harmless looking, huh?
		status_t RunChain(bool asynchronous = true);
		
		void MessageReceived (BMessage *msg);

	private:
		void CallCallbacksFor(BList &list, status_t code);
		void get_messages(BStringList *list);
		status_t Init();
		#if USE_NASTY_SYNC_THREAD_HACK
			static int32 thread_sync_func(void *arg);
			status_t init_addons();
		#endif
	
		BMailChain *_chain;
		BList message_cb, process_cb, chain_cb;
		
		bool destroy_self, destroy_chain, save_chain;
		
		BMailStatusWindow *_status;
		BMailStatusView *_statview;
		BList addons;
		
		bool suicide;
		uint8	_other_reserved[3];
		uint32	_reserved[4];
};

BMailChainRunner *GetMailChainRunner(int32 chain_id, BMailStatusWindow *status, bool selfDestruct = true);

#endif	/* ZOIDBERG_MAIL_CHAIN_RUNNER_H */
