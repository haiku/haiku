//
// Class to encapsulate entries in the InputServer's InputServerMethod list.
//
class InputServerMethodListEntry
{
	public:
		inline const char*               getPath()              { return mPath;   };		
		inline status_t                  getStatus()            { return mStatus; };
		inline const BInputServerMethod* getInputServerMethod() { return mIsm;    };
		
		InputServerMethodListEntry(
			const char*               path,
			status_t                  status,
			const BInputServerMethod* ism) : mPath  (path),
			                                 mStatus(status),
			                                 mIsm   (ism)
		{ /* Do nothing */ };
				
		~InputServerMethodListEntry()
		{ /* Do nothing */ };

	private:	
		const char*               mPath;
		status_t                  mStatus;
		const BInputServerMethod* mIsm;
	
};
