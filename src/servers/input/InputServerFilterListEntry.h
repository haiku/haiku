//
// Class to encapsulate entries in the InputServer's InputServerFilter list.
//
class InputServerFilterListEntry
{
	public:
		inline const char*               getPath()              { return mPath;   };		
		inline status_t                  getStatus()            { return mStatus; };
		inline const BInputServerFilter* getInputServerFilter() { return mIsd;    };
		
		InputServerFilterListEntry(
			const char*               path,
			status_t                  status,
			const BInputServerFilter* isd) : mPath  (path),
			                                 mStatus(status),
			                                 mIsd   (isd)
		{ /* Do nothing */ };
				
		~InputServerFilterListEntry()
		{ /* Do nothing */ };

	private:	
		const char*               mPath;
		status_t                  mStatus;
		const BInputServerFilter* mIsd;
	
};
