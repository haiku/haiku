//
// Class to encapsulate entries in the InputServer's InputServerDevice list.
//
class InputServerDeviceListEntry
{
	public:
		inline const char*               getPath()              { return mPath;   };		
		inline status_t                  getStatus()            { return mStatus; };
		inline const BInputServerDevice* getInputServerDevice() { return mIsd;    };
		
		InputServerDeviceListEntry(
			const char*               path,
			status_t                  status,
			const BInputServerDevice* isd) : mPath  (path),
			                                 mStatus(status),
			                                 mIsd   (isd)
		{ /* Do nothing */ };
				
		~InputServerDeviceListEntry()
		{ /* Do nothing */ };

	private:	
		const char*               mPath;
		status_t                  mStatus;
		const BInputServerDevice* mIsd;
	
};
